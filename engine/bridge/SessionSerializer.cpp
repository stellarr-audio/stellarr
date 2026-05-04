#include "../StellarrProcessor.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"
#include "SceneCapture.h"
#include "internal/BlockLifecycle.h"
#include <mutex>

using namespace stellarr::bridge::internal;

// -- Session serialization ----------------------------------------------------

juce::var StellarrBridge::serialiseSession() const
{
    if (processor == nullptr) return {};

    auto* session = new juce::DynamicObject();
    session->setProperty("version", 1);

    auto* gridObj = new juce::DynamicObject();
    gridObj->setProperty("columns", gridCols);
    gridObj->setProperty("rows", gridRows);
    session->setProperty("grid", juce::var(gridObj));

    // Blocks
    juce::Array<juce::var> blocksArray;
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                auto blockJson = block->toJson();
                if (auto* obj = blockJson.getDynamicObject())
                {
                    auto posIt = blockPositions.find(blockId);
                    if (posIt != blockPositions.end())
                    {
                        obj->setProperty("col", posIt->second.first);
                        obj->setProperty("row", posIt->second.second);
                    }
                }
                blocksArray.add(blockJson);
            }
        }
    }
    session->setProperty("blocks", blocksArray);

    // Connections
    juce::Array<juce::var> connectionsArray;
    std::map<juce::uint32, juce::String> nodeToBlock;
    for (auto& [blockId, nodeId] : blockNodeMap)
        nodeToBlock[nodeId.uid] = blockId;

    for (auto& conn : processor->getGraph().getConnections())
    {
        if (conn.source.channelIndex != 0) continue;
        auto srcIt = nodeToBlock.find(conn.source.nodeID.uid);
        auto dstIt = nodeToBlock.find(conn.destination.nodeID.uid);
        if (srcIt == nodeToBlock.end() || dstIt == nodeToBlock.end()) continue;

        auto* connObj = new juce::DynamicObject();
        connObj->setProperty("sourceId", srcIt->second);
        connObj->setProperty("destId", dstIt->second);
        connectionsArray.add(juce::var(connObj));
    }
    session->setProperty("connections", connectionsArray);

    // Update active scene before serialising
    if (activeSceneIndex >= 0 && activeSceneIndex < static_cast<int>(scenes.size()))
        captureIntoScene(const_cast<StellarrBridge*>(this)->scenes[static_cast<size_t>(activeSceneIndex)],
                                   blockNodeMap, processor->getGraph());

    // Scenes
    juce::Array<juce::var> scenesArray;
    for (auto& scene : scenes)
    {
        auto* sceneObj = new juce::DynamicObject();
        sceneObj->setProperty("name", scene.name);
        auto* mapObj = new juce::DynamicObject();
        for (auto& [bid, si] : scene.blockStateMap)
            mapObj->setProperty(bid, si);
        sceneObj->setProperty("blockStateMap", juce::var(mapObj));
        auto* bypassObj = new juce::DynamicObject();
        for (auto& [bid, bp] : scene.blockBypassMap)
            bypassObj->setProperty(bid, bp);
        sceneObj->setProperty("blockBypassMap", juce::var(bypassObj));
        scenesArray.add(juce::var(sceneObj));
    }
    session->setProperty("scenes", scenesArray);
    session->setProperty("activeSceneIndex", activeSceneIndex);

    // MIDI mappings (preset-level only — global mappings stored in app settings)
    if (processor != nullptr)
        session->setProperty("midiMappings", processor->getMidiMapper().presetMappingsToJson());

    return juce::var(session);
}

void StellarrBridge::clearGraph()
{
    using UK = juce::AudioProcessorGraph::UpdateKind;

    // Close plugin editor windows first — removeBlock deletes the processor,
    // which would leave dangling window references.
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                pluginBlock->closePluginEditor();
    }

    // Note: MIDI mappings are NOT pruned per-block here. loadPresetMappings()
    // further down replaces all preset-level mappings atomically; pruning
    // here would just churn the mapping list before the replacement.

    // Remove all blocks without rebuilding after each one.
    auto ids = blockNodeMap;
    for (auto& [blockId, nodeId] : ids)
        processor->removeBlock(nodeId, UK::none);

    blockNodeMap.clear();
    blockPositions.clear();
}

bool StellarrBridge::restoreSession(juce::var session,
                                    std::function<void(bool)> onComplete)
{
    if (processor == nullptr) return false;

    auto* obj = session.getDynamicObject();
    if (obj == nullptr) return false;

    // Guard against rapid-fire concurrent invocations (e.g. user mashing the
    // preset selector). A lost try-lock means another restore is already in
    // flight — drop this one rather than queue a second graph rebuild on top.
    // Callers must inspect the return value before mutating any "current
    // preset" bookkeeping, otherwise a quiet save could persist the previous
    // graph into the file the user thinks is the new preset.
    //
    // Same-thread reentrancy short-circuit FIRST: with the async cooperative
    // load, the message thread returns between AsyncUpdater ticks and may
    // re-enter restoreSession (e.g. drainMidiEvents → onPresetChange →
    // handleLoadPresetByIndex → restoreSession). Calling try_lock from the
    // thread that already owns the lock is UB on a non-recursive std::mutex,
    // so reject via the atomic flag before we touch the mutex. The flag
    // mirrors "a restore is in flight" and is set under the mutex below;
    // reading it here is well-defined regardless of which thread we're on
    // because std::atomic carries its own memory ordering.
    if (restoreInProgress.load(std::memory_order_acquire))
    {
        DBG("restoreSession: rejected reentrant request (load already in flight)");
        return false;
    }

    std::unique_lock<std::mutex> lock(restoreMutex, std::try_to_lock);
    if (!lock.owns_lock())
    {
        DBG("restoreSession: rejected reentrant request");
        return false;
    }

    // Mark in-flight under the mutex so the next reentrant call sees it.
    restoreInProgress.store(true, std::memory_order_release);

    // RAII rollback in case any exception fires between here and pendingRestore
    // taking ownership. Without this, an alloc / juce::var copy throw during
    // pluginBlocks setup would unwind the local lock but leave restoreInProgress
    // pinned true, permanently rejecting future loads via the atomic fast-path.
    struct InProgressRollback {
        std::atomic<bool>& flag;
        bool committed = false;
        ~InProgressRollback() {
            if (!committed) flag.store(false, std::memory_order_release);
        }
    };
    InProgressRollback rollback{restoreInProgress};

    // Build the ordered list of plugin blocks up-front so handleAsyncUpdate()
    // can step through them one tick at a time without re-parsing each call.
    std::vector<juce::var> pluginBlocks;
    auto blocksVar = obj->getProperty("blocks");
    if (auto* blocksArray = blocksVar.getArray())
    {
        for (auto& blockVar : *blocksArray)
        {
            auto* blockObj = blockVar.getDynamicObject();
            if (blockObj == nullptr) continue;

            auto type = blockObj->getProperty("type").toString();
            if (type != "plugin" && type != "vst") continue;

            auto pluginId = blockObj->getProperty("pluginId").toString();
            if (pluginId.isEmpty()) continue;

            pluginBlocks.push_back(blockVar);
        }
    }

    pendingRestore.emplace(PendingRestore{
        std::move(lock),
        std::move(session),
        {},                  // preloads — accumulated tick by tick
        0,                   // nextIndex
        std::move(onComplete),
        std::move(pluginBlocks)
    });

    // pendingRestore now owns the in-flight invariant via RestoreTeardown +
    // the catch path below; release the rollback guard.
    rollback.committed = true;

    // Emit the start bracket. If the interceptor or webview emit throws,
    // roll back the pendingRestore so the lock isn't held forever — the
    // caller's `started==true` contract is then a lie, but failing fast and
    // re-throwing is preferable to a permanently-wedged bridge.
    try
    {
        emitSync("presetLoadStarted", new juce::DynamicObject());
    }
    catch (...)
    {
        // Order: release the mutex BEFORE clearing the flag (see RestoreTeardown
        // for the same-thread reentry rationale).
        pendingRestore.reset();   // releases lock via unique_lock dtor
        restoreInProgress.store(false, std::memory_order_release);
        throw;
    }

    // Schedule the first tick. Even when there are zero plugins to load we
    // still defer Phase 2 to a tick so the UI has a chance to paint the
    // spinner before the graph rebuild runs.
    triggerAsyncUpdate();
    return true;
}

void StellarrBridge::handleAsyncUpdate()
{
    // Defensive: should always be set when a tick fires, but if something
    // cancelled out from under us (e.g. cleared during shutdown) just bail.
    if (!pendingRestore.has_value()) return;

    auto& pr = *pendingRestore;

    // Phase 1: load one plugin per tick. Each createPluginInstance call may
    // block the message thread for hundreds of ms; yielding between them lets
    // the UI paint and process input.
    if (pr.nextIndex < pr.pluginBlocks.size())
    {
        auto& blockVar = pr.pluginBlocks[pr.nextIndex];
        auto* blockObj = blockVar.getDynamicObject();
        // Already filtered in restoreSession() but guard again defensively.
        if (blockObj != nullptr)
        {
            auto savedId    = blockObj->getProperty("id").toString();
            auto pluginId   = blockObj->getProperty("pluginId").toString();
            auto pluginName = blockObj->getProperty("pluginName").toString();

            std::unique_ptr<juce::AudioPluginInstance> instance;
            try
            {
                juce::String errorMessage;
                instance = processor->getPluginManager().createPluginInstance(
                    pluginId, processor->getSampleRate(),
                    processor->getBlockSize(), errorMessage);

                if (instance != nullptr)
                {
                    instance->setPlayConfigDetails(2, 2, processor->getSampleRate(),
                                                   processor->getBlockSize());
                    instance->prepareToPlay(processor->getSampleRate(),
                                            processor->getBlockSize());
                }
            }
            catch (...)
            {
                // Single-plugin failure must not kill the whole restore — the
                // session may have many other working plugins. Record nullptr
                // so Phase 2 sets the missing-plugin marker, mirroring the
                // existing "instance == nullptr" branch.
                instance.reset();
                DBG("restoreSession: plugin load threw for " << pluginId);
            }

            pr.preloads.push_back({savedId, pluginId, pluginName, std::move(instance)});
        }

        ++pr.nextIndex;
        triggerAsyncUpdate();
        return;
    }

    // Phase 2: all plugins pre-loaded — install everything in one suspended
    // window, then notify and release.
    bool success = false;
    try
    {
        success = finishRestore();
    }
    catch (...)
    {
        success = false;
        DBG("restoreSession: finishRestore threw");
    }

    // Move the user callback off of pendingRestore before any further work
    // touches it — if the bracket emit or callback throws, the RAII cleanup
    // below still resets state and drains continuations.
    auto callback = std::move(pr.onComplete);

    // RAII teardown — guarantees the restore lock is released and any
    // post-restore continuations fire even if presetLoadFinished or the
    // user callback throws (e.g. via the test emit interceptor).
    struct RestoreTeardown {
        StellarrBridge* self;
        ~RestoreTeardown()
        {
            // Reset pendingRestore FIRST so the unique_lock destructor releases
            // restoreMutex before we clear the atomic. If we cleared the flag
            // first and a destructor in pendingRestore re-entered restoreSession
            // on the same thread, the short-circuit would miss and try_lock on
            // an already-owned non-recursive mutex (UB).
            self->pendingRestore.reset();   // releases lock via unique_lock dtor
            self->restoreInProgress.store(false, std::memory_order_release);
            auto continuations = std::move(self->postRestoreContinuations);
            self->postRestoreContinuations.clear();
            for (auto& cont : continuations)
            {
                if (!cont) continue;
                try { cont(); }
                catch (...) { DBG("restoreSession: post-restore continuation threw"); }
            }
        }
    } teardown{this};

    // Use async emit (not sync) so presetLoadFinished is delivered AFTER the
    // queued graphState / gridState / scene / MIDI mapping updates emitted
    // during finishRestore(). The UI's loading-state flag gates pointer
    // events on the preset surfaces — clearing it before the new graph
    // arrives lets the user click on stale UI for a few frames.
    emit("presetLoadFinished", new juce::DynamicObject());

    if (callback) callback(success);
}

void StellarrBridge::runWhenRestoreIdle(std::function<void()> cb)
{
    if (!cb) return;
    if (!pendingRestore.has_value())
    {
        cb();
        return;
    }
    postRestoreContinuations.push_back(std::move(cb));
}

bool StellarrBridge::finishRestore()
{
    if (!pendingRestore.has_value()) return false;
    auto& pr = *pendingRestore;

    auto* obj = pr.session.getDynamicObject();
    if (obj == nullptr) return false;

    using UK = juce::AudioProcessorGraph::UpdateKind;

    // Mutate the graph inside a tight suspended window. RAII resume guard
    // covers exceptions thrown by plugin state restore so audio still resumes
    // even on failure — but it also calls rebuildGraph() defensively first so
    // the audio thread never sees a partial mutation (cleared blocks, no
    // fresh render sequence). Scope is intentionally narrow: block/connection
    // install plus the single rebuild plus the MIDI mapping swap — everything
    // afterwards (scenes, grid, UI emits) runs with audio live again to keep
    // the gap minimal.
    {
        processor->suspendProcessing(true);
        struct ResumeGuard {
            StellarrProcessor* p;
            bool rebuilt = false;
            ~ResumeGuard()
            {
                if (p == nullptr) return;
                // Defensive rebuild only if the body threw before its
                // explicit rebuildGraph() ran. Avoids duplicating the
                // expensive rebuild on the success path.
                if (!rebuilt)
                {
                    try { p->rebuildGraph(); } catch (...) {}
                }
                p->suspendProcessing(false);
            }
        } resumeGuard{processor, false};

        clearGraph();

        auto blocksVar = obj->getProperty("blocks");
        if (auto* blocksArray = blocksVar.getArray())
        {
            // Build a lookup from saved block ID to preloaded instance
            std::map<juce::String, size_t> preloadIndex;
            for (size_t i = 0; i < pr.preloads.size(); ++i)
                preloadIndex[pr.preloads[i].blockId] = i;

            for (auto& blockVar : *blocksArray)
            {
                auto* blockObj = blockVar.getDynamicObject();
                if (blockObj == nullptr) continue;

                auto type = blockObj->getProperty("type").toString();
                auto col  = static_cast<int>(blockObj->getProperty("col"));
                auto row  = static_cast<int>(blockObj->getProperty("row"));
                auto savedId = blockObj->getProperty("id").toString();

                std::unique_ptr<stellarr::Block> block;
                if (type == "input")       block = std::make_unique<stellarr::InputBlock>();
                else if (type == "output") block = std::make_unique<stellarr::OutputBlock>();
                else if (type == "plugin" || type == "vst")  block = std::make_unique<stellarr::PluginBlock>();
                else continue;

                block->fromJson(blockVar);
                block->resetToDefault();

                auto blockId = savedId.isNotEmpty() ? savedId : block->getBlockId().toString();
                auto nodeId = processor->addBlock(std::move(block), UK::none);
                if (nodeId.uid == 0) continue;

                blockNodeMap[blockId] = nodeId;
                blockPositions[blockId] = {col, row};

                connectIOBlock(*processor, type, nodeId, UK::none);

                // Install pre-loaded plugin instance (just a pointer swap, fast)
                if (type == "plugin" || type == "vst")
                {
                    auto preloadIt = preloadIndex.find(blockId);
                    if (preloadIt != preloadIndex.end())
                    {
                        auto& pl = pr.preloads[preloadIt->second];
                        if (auto* node = processor->getGraph().getNodeForId(nodeId))
                        {
                            if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                            {
                                if (pl.instance != nullptr)
                                {
                                    pb->setPlugin(std::move(pl.instance), pl.pluginId);
                                    pb->restorePluginState();
                                }
                                else
                                {
                                    pb->setPluginMissing(true);
                                    pb->setMissingPluginName(pl.pluginName);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Restore connections (also batched)
        auto connectionsVar = obj->getProperty("connections");
        if (auto* connectionsArray = connectionsVar.getArray())
        {
            for (auto& connVar : *connectionsArray)
            {
                auto* connObj = connVar.getDynamicObject();
                if (connObj == nullptr) continue;

                auto sourceId = connObj->getProperty("sourceId").toString();
                auto destId   = connObj->getProperty("destId").toString();

                auto srcIt = blockNodeMap.find(sourceId);
                auto dstIt = blockNodeMap.find(destId);
                if (srcIt == blockNodeMap.end() || dstIt == blockNodeMap.end()) continue;

                processor->connectBlocks(srcIt->second, dstIt->second, 2, UK::none);
            }
        }

        // Single atomic rebuild — audio thread picks up the complete new graph in one swap
        processor->rebuildGraph();
        resumeGuard.rebuilt = true;

        // Swap preset-level MIDI mappings inside the suspended window so the
        // audio thread never processes a block against the previous preset's
        // mappings — otherwise an inbound CC arriving in the gap could
        // mutate the wrong block.
        processor->getMidiMapper().loadPresetMappings(
            obj->hasProperty("midiMappings") ? obj->getProperty("midiMappings") : juce::var());
    }   // ResumeGuard fires here — audio is live again before the UI emits below.

    // Restore scenes
    scenes.clear();
    activeSceneIndex = -1;
    auto scenesVar = obj->getProperty("scenes");
    if (auto* scenesArr = scenesVar.getArray())
    {
        for (auto& sv : *scenesArr)
        {
            if (auto* so = sv.getDynamicObject())
            {
                Scene scene;
                scene.name = so->getProperty("name").toString();
                auto mapVar = so->getProperty("blockStateMap");
                if (auto* mapObj = mapVar.getDynamicObject())
                {
                    for (auto& prop : mapObj->getProperties())
                        scene.blockStateMap[prop.name.toString()] = static_cast<int>(prop.value);
                }
                auto bypassVar = so->getProperty("blockBypassMap");
                if (auto* bypassObj = bypassVar.getDynamicObject())
                {
                    for (auto& prop : bypassObj->getProperties())
                        scene.blockBypassMap[prop.name.toString()] = static_cast<bool>(prop.value);
                }
                scenes.push_back(scene);
            }
        }
        activeSceneIndex = static_cast<int>(obj->getProperty("activeSceneIndex"));
        if (activeSceneIndex >= static_cast<int>(scenes.size()))
            activeSceneIndex = scenes.empty() ? -1 : 0;
    }

    // Ensure at least one scene exists
    if (scenes.empty())
    {
        Scene defaultScene;
        defaultScene.name = "Scene 1";
        captureIntoScene(defaultScene, blockNodeMap, processor->getGraph());
        scenes.push_back(defaultScene);
        activeSceneIndex = 0;
    }

    // Mappings were swapped atomically inside the suspended window; this
    // emit just informs the UI of the new mapping list.
    emitMidiMappings();

    // Restore grid dimensions (falls back to current defaults if absent)
    if (obj->hasProperty("grid"))
    {
        if (auto* gridObj = obj->getProperty("grid").getDynamicObject())
        {
            auto cols = gridObj->getProperty("columns");
            auto rows = gridObj->getProperty("rows");
            if (cols.isInt() || cols.isInt64() || cols.isDouble())
                gridCols = static_cast<int>(cols);
            if (rows.isInt() || rows.isInt64() || rows.isDouble())
                gridRows = static_cast<int>(rows);
        }
    }

    sendGraphState();
    emitGridState();
    return true;
}
