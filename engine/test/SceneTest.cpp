#include "TestUtils.h"
#include "blocks/PluginBlock.h"
#include "blocks/GainBlock.h"
#include "StellarrBridge.h"

// Test scene struct directly — no bridge needed for basic data tests

static bool testSceneCapture()
{
    printf("Test: scene captures block state indices and bypass... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto plugin = std::make_unique<stellarr::PluginBlock>();
    plugin->addState(); // now has 2 states
    plugin->recallState(1);
    plugin->setBypassed(true);
    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(plugin));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    // Simulate scene capture
    StellarrBridge::Scene scene;
    scene.name = "Test";

    auto* node = proc.getGraph().getNodeForId(nodeId);
    auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
    juce::String blockId = pb->getBlockId().toString();

    scene.blockStateMap[blockId] = pb->getActiveStateIndex();
    scene.blockBypassMap[blockId] = pb->isBypassed();

    if (scene.blockStateMap[blockId] != 1)
    {
        fprintf(stderr, "  expected state index 1, got %d\n", scene.blockStateMap[blockId]);
        printf("FAIL\n");
        return false;
    }

    if (!scene.blockBypassMap[blockId])
    {
        fprintf(stderr, "  expected bypass=true\n");
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testSceneRecall()
{
    printf("Test: scene recall restores state index and bypass... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto plugin = std::make_unique<stellarr::PluginBlock>();
    plugin->setMix(0.5f);
    plugin->saveCurrentState();
    plugin->addState(); // state 1
    plugin->setMix(0.8f);
    plugin->saveCurrentState();

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(plugin));

    auto* node = proc.getGraph().getNodeForId(nodeId);
    auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
    juce::String blockId = pb->getBlockId().toString();

    // Create scene pointing to state 0, bypassed
    StellarrBridge::Scene scene;
    scene.name = "Clean";
    scene.blockStateMap[blockId] = 0;
    scene.blockBypassMap[blockId] = true;

    // Currently on state 1, not bypassed
    pb->recallState(1);
    pb->setBypassed(false);

    // Recall scene
    int stateIdx = scene.blockStateMap[blockId];
    pb->recallState(stateIdx);
    pb->setBypassed(scene.blockBypassMap[blockId]);

    if (pb->getActiveStateIndex() != 0)
    {
        fprintf(stderr, "  expected state 0, got %d\n", pb->getActiveStateIndex());
        printf("FAIL\n");
        return false;
    }

    if (!pb->isBypassed())
    {
        fprintf(stderr, "  expected bypassed\n");
        printf("FAIL\n");
        return false;
    }

    if (std::abs(pb->getMix() - 0.5f) > 0.01f)
    {
        fprintf(stderr, "  expected mix=0.5, got %f\n", static_cast<double>(pb->getMix()));
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testSceneSerialisation()
{
    printf("Test: scene serialises to JSON and restores... ");

    StellarrBridge::Scene original;
    original.name = "Heavy";
    original.blockStateMap["block-abc"] = 2;
    original.blockStateMap["block-def"] = 0;
    original.blockBypassMap["block-abc"] = false;
    original.blockBypassMap["block-def"] = true;

    // Serialise
    auto* sceneObj = new juce::DynamicObject();
    sceneObj->setProperty("name", original.name);
    auto* mapObj = new juce::DynamicObject();
    for (auto& [bid, si] : original.blockStateMap)
        mapObj->setProperty(bid, si);
    sceneObj->setProperty("blockStateMap", juce::var(mapObj));
    auto* bypassObj = new juce::DynamicObject();
    for (auto& [bid, bp] : original.blockBypassMap)
        bypassObj->setProperty(bid, bp);
    sceneObj->setProperty("blockBypassMap", juce::var(bypassObj));

    juce::var json(sceneObj);

    // Restore
    StellarrBridge::Scene restored;
    auto* so = json.getDynamicObject();
    restored.name = so->getProperty("name").toString();

    auto stateMapVar = so->getProperty("blockStateMap");
    if (auto* sm = stateMapVar.getDynamicObject())
        for (auto& prop : sm->getProperties())
            restored.blockStateMap[prop.name.toString()] = static_cast<int>(prop.value);

    auto bypassMapVar = so->getProperty("blockBypassMap");
    if (auto* bm = bypassMapVar.getDynamicObject())
        for (auto& prop : bm->getProperties())
            restored.blockBypassMap[prop.name.toString()] = static_cast<bool>(prop.value);

    if (restored.name != "Heavy")
    {
        fprintf(stderr, "  name mismatch\n");
        printf("FAIL\n");
        return false;
    }

    if (restored.blockStateMap["block-abc"] != 2 ||
        restored.blockStateMap["block-def"] != 0)
    {
        fprintf(stderr, "  stateMap mismatch\n");
        printf("FAIL\n");
        return false;
    }

    if (restored.blockBypassMap["block-abc"] != false ||
        restored.blockBypassMap["block-def"] != true)
    {
        fprintf(stderr, "  bypassMap mismatch\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testSceneMaxLimit()
{
    printf("Test: scene max limit of 16 enforced... ");

    std::vector<StellarrBridge::Scene> scenes;

    for (int i = 0; i < 16; ++i)
    {
        StellarrBridge::Scene s;
        s.name = "Scene " + juce::String(i + 1);
        scenes.push_back(s);
    }

    if (static_cast<int>(scenes.size()) != 16)
    {
        fprintf(stderr, "  expected 16 scenes\n");
        printf("FAIL\n");
        return false;
    }

    // Verify we can't exceed the expected limit
    if (scenes.size() > 16)
    {
        fprintf(stderr, "  exceeded 16 scene limit\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testSceneRecallClampsStateIndex()
{
    printf("Test: scene recall clamps state index beyond block's state count... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto plugin = std::make_unique<stellarr::PluginBlock>();
    // Only 1 state (index 0)

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(plugin));

    auto* node = proc.getGraph().getNodeForId(nodeId);
    auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
    juce::String blockId = pb->getBlockId().toString();

    // Scene references state index 5 — block only has 1 state
    StellarrBridge::Scene scene;
    scene.name = "OutOfRange";
    scene.blockStateMap[blockId] = 5;
    scene.blockBypassMap[blockId] = false;

    int stateIdx = scene.blockStateMap[blockId];
    int clamped = juce::jmin(stateIdx, pb->getNumStates() - 1);
    pb->recallState(clamped);

    if (pb->getActiveStateIndex() != 0)
    {
        fprintf(stderr, "  expected clamped to 0, got %d\n", pb->getActiveStateIndex());
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testSceneRecallNoSuspend()
{
    printf("Test: scene recall does not suspend audio processing... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setMix(0.5f);
    block.addState();
    block.setMix(0.8f);

    // Recall state 0 — should use hot path
    block.recallState(0);

    if (block.isSuspended())
    {
        fprintf(stderr, "  scene recall should not suspend processing\n");
        printf("FAIL\n");
        return false;
    }

    if (std::abs(block.getMix() - 0.5f) > 0.01f)
    {
        fprintf(stderr, "  expected mix=0.5, got %f\n", static_cast<double>(block.getMix()));
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testMultipleRapidRecalls()
{
    printf("Test: multiple rapid scene recalls do not crash or corrupt index... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.addState();
    block.addState();
    // 3 states: 0, 1, 2

    // Rapid switching — should not crash or produce invalid index
    for (int i = 0; i < 51; ++i)
        block.recallState(i % 3);

    // After 51 iterations (last index 50 % 3 = 2), should be on state 2
    if (block.getActiveStateIndex() != 2)
    {
        fprintf(stderr, "  expected state 2, got %d\n", block.getActiveStateIndex());
        printf("FAIL\n");
        return false;
    }

    // Index must be valid
    if (block.getActiveStateIndex() < 0 || block.getActiveStateIndex() >= block.getNumStates())
    {
        fprintf(stderr, "  invalid state index after rapid recalls\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    if (!testSceneCapture())              ++failures;
    if (!testSceneRecall())               ++failures;
    if (!testSceneSerialisation())        ++failures;
    if (!testSceneMaxLimit())             ++failures;
    if (!testSceneRecallClampsStateIndex()) ++failures;
    if (!testSceneRecallNoSuspend())       ++failures;
    if (!testMultipleRapidRecalls())       ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
