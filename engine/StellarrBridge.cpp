#include "StellarrBridge.h"
#include "StellarrProcessor.h"
#include "blocks/InputBlock.h"
#include "blocks/OutputBlock.h"
#include "blocks/VstBlock.h"

StellarrBridge::StellarrBridge() = default;

void StellarrBridge::setProcessor(StellarrProcessor* proc)
{
    processor = proc;
}

juce::WebBrowserComponent::Options StellarrBridge::configureOptions(juce::WebBrowserComponent::Options options)
{
    return options.withNativeFunction("sendToNative",
        [this](const juce::Array<juce::var>& args,
               juce::WebBrowserComponent::NativeFunctionCompletion completion)
        {
            if (args.size() >= 2)
                handleEvent(args[0].toString(), args[1]);
            else if (args.size() == 1)
                handleEvent(args[0].toString(), {});

            completion(juce::var("ok"));
        });
}

void StellarrBridge::setWebView(juce::WebBrowserComponent* browser)
{
    webView = browser;
}

void StellarrBridge::handleEvent(const juce::String& eventName, const juce::var& payload)
{
    auto json = payload.isString()
        ? juce::JSON::parse(payload.toString())
        : payload;

    if (eventName == "bridgeReady")
        handleBridgeReady();
    else if (eventName == "addBlock")            handleAddBlock(json);
    else if (eventName == "removeBlock")         handleRemoveBlock(json);
    else if (eventName == "moveBlock")           handleMoveBlock(json);
    else if (eventName == "addConnection")       handleAddConnection(json);
    else if (eventName == "removeConnection")    handleRemoveConnection(json);
    else if (eventName == "scanPlugins")         handleScanPlugins();
    else if (eventName == "getScanDirectories")  handleGetScanDirectories();
    else if (eventName == "pickScanDirectory")   handlePickScanDirectory();
    else if (eventName == "removeScanDirectory") handleRemoveScanDirectory(json);
    else if (eventName == "toggleTestTone" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId = obj->getProperty("blockId").toString();
        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
            {
                bool enabled = !inputBlock->isTestToneEnabled();
                inputBlock->setTestToneEnabled(enabled);

                auto* detail = new juce::DynamicObject();
                detail->setProperty("blockId", blockId);
                detail->setProperty("enabled", enabled);
                emitToJs("testToneChanged", detail);
            }
        }
    }
    else if (eventName == "uiAction" && webView != nullptr)
    {
        auto* response = new juce::DynamicObject();
        response->setProperty("message", "Pong: " + payload.toString());
        webView->emitEventIfBrowserIsVisible("pong", juce::var(response));
    }
}

void StellarrBridge::emitToJs(const juce::String& eventName, juce::DynamicObject* detail)
{
    if (webView == nullptr) return;

    // Defer emission so it runs after the native function callback returns.
    // emitEventIfBrowserIsVisible may not deliver events re-entrantly
    // from within a WKWebView message handler.
    auto eventId = juce::Identifier(eventName);
    auto data = juce::var(detail);

    juce::MessageManager::callAsync([this, eventId, data]()
    {
        if (webView != nullptr)
            webView->emitEventIfBrowserIsVisible(eventId, data);
    });
}

void StellarrBridge::handleBridgeReady()
{
    sendWelcome();

    // Create default Input and Output blocks if the graph is empty
    if (blockNodeMap.empty())
    {
        auto inputJson = juce::JSON::parse(R"({"type":"input","col":0,"row":2})");
        auto outputJson = juce::JSON::parse(R"({"type":"output","col":11,"row":2})");
        handleAddBlock(inputJson);
        handleAddBlock(outputJson);
    }

    sendGraphState();
    sendScanDirectories();
}

// -- Graph event handlers ----------------------------------------------------

void StellarrBridge::handleAddBlock(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto type = obj->getProperty("type").toString();
    auto col  = static_cast<int>(obj->getProperty("col"));
    auto row  = static_cast<int>(obj->getProperty("row"));

    std::unique_ptr<stellarr::Block> block;

    if (type == "input")       block = std::make_unique<stellarr::InputBlock>();
    else if (type == "output") block = std::make_unique<stellarr::OutputBlock>();
    else if (type == "vst")    block = std::make_unique<stellarr::VstBlock>();
    else return;

    auto blockId = block->getBlockId().toString();
    auto blockName = block->getName();
    auto nodeId = processor->addBlock(std::move(block));

    if (nodeId.uid == 0) return;

    blockNodeMap[blockId] = nodeId;
    blockPositions[blockId] = {col, row};

    // Auto-wire user I/O blocks to the graph's built-in audio I/O nodes.
    // Remove the default built-in pass-through so it does not compete.
    if (type == "input")
    {
        processor->disconnectBlocks(processor->getAudioInputNodeId(), processor->getAudioOutputNodeId());
        processor->connectBlocks(processor->getAudioInputNodeId(), nodeId);
    }
    else if (type == "output")
    {
        processor->disconnectBlocks(processor->getAudioInputNodeId(), processor->getAudioOutputNodeId());
        processor->connectBlocks(nodeId, processor->getAudioOutputNodeId());
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("id", blockId);
    detail->setProperty("type", type);
    detail->setProperty("name", blockName);
    detail->setProperty("col", col);
    detail->setProperty("row", row);
    detail->setProperty("nodeId", static_cast<int>(nodeId.uid));
    emitToJs("blockAdded", detail);
}

void StellarrBridge::handleRemoveBlock(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto it = blockNodeMap.find(blockId);
    if (it == blockNodeMap.end()) return;

    // Remove all connections involving this block
    processor->removeBlock(it->second);
    blockNodeMap.erase(it);
    blockPositions.erase(blockId);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    emitToJs("blockRemoved", detail);
}

void StellarrBridge::handleMoveBlock(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto col = static_cast<int>(obj->getProperty("col"));
    auto row = static_cast<int>(obj->getProperty("row"));

    blockPositions[blockId] = {col, row};

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("col", col);
    detail->setProperty("row", row);
    emitToJs("blockMoved", detail);
}

void StellarrBridge::handleAddConnection(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto sourceId = obj->getProperty("sourceId").toString();
    auto destId   = obj->getProperty("destId").toString();

    auto srcIt = blockNodeMap.find(sourceId);
    auto dstIt = blockNodeMap.find(destId);
    if (srcIt == blockNodeMap.end() || dstIt == blockNodeMap.end()) return;

    if (!processor->connectBlocks(srcIt->second, dstIt->second))
        return;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("sourceId", sourceId);
    detail->setProperty("destId", destId);
    emitToJs("connectionAdded", detail);
}

void StellarrBridge::handleRemoveConnection(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto sourceId = obj->getProperty("sourceId").toString();
    auto destId   = obj->getProperty("destId").toString();

    auto srcIt = blockNodeMap.find(sourceId);
    auto dstIt = blockNodeMap.find(destId);
    if (srcIt == blockNodeMap.end() || dstIt == blockNodeMap.end()) return;

    processor->disconnectBlocks(srcIt->second, dstIt->second);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("sourceId", sourceId);
    detail->setProperty("destId", destId);
    emitToJs("connectionRemoved", detail);
}

// -- Plugin management -------------------------------------------------------

void StellarrBridge::handleScanPlugins()
{
    if (processor == nullptr) return;

    processor->getPluginManager().scanPlugins();
    sendPluginList();
}

void StellarrBridge::handleGetScanDirectories()
{
    sendScanDirectories();
}

void StellarrBridge::handlePickScanDirectory()
{
    if (processor == nullptr) return;

    // Defer the file chooser so it runs after the native function callback returns.
    // FileChooser::launchAsync does not work from within a WKWebView message handler.
    juce::MessageManager::callAsync([this]()
    {
        if (processor == nullptr) return;

        auto chooser = std::make_shared<juce::FileChooser>("Select Plugin Directory");

        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this, chooser](const juce::FileChooser&)
            {
                auto result = chooser->getResult();
                if (result == juce::File{}) return;

                processor->getPluginManager().addScanDirectory(result.getFullPathName());
                sendScanDirectories();
            });
    });
}

void StellarrBridge::handleRemoveScanDirectory(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto path = obj->getProperty("path").toString();
    processor->getPluginManager().removeScanDirectory(path);
    sendScanDirectories();
}

void StellarrBridge::sendPluginList()
{
    if (processor == nullptr) return;

    juce::Array<juce::var> plugins;
    for (auto& desc : processor->getPluginManager().getKnownPlugins().getTypes())
    {
        auto* p = new juce::DynamicObject();
        p->setProperty("id", desc.createIdentifierString());
        p->setProperty("name", desc.name);
        p->setProperty("manufacturer", desc.manufacturerName);
        p->setProperty("format", desc.pluginFormatName);
        plugins.add(juce::var(p));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("plugins", plugins);
    emitToJs("pluginListUpdated", detail);
}

void StellarrBridge::sendScanDirectories()
{
    if (processor == nullptr) return;

    juce::Array<juce::var> dirs;
    for (auto& d : processor->getPluginManager().getScanDirectories())
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("path", d.path);
        obj->setProperty("isDefault", d.isDefault);
        dirs.add(juce::var(obj));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("directories", dirs);
    emitToJs("scanDirectoriesUpdated", detail);
}

// -- Welcome and graph state -------------------------------------------------

void StellarrBridge::sendWelcome()
{
    if (webView == nullptr) return;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("message", "Stellarr C++ engine is running");
    webView->emitEventIfBrowserIsVisible("welcome", juce::var(detail));
}

void StellarrBridge::sendGraphState()
{
    if (webView == nullptr) return;

    juce::Array<juce::var> blocksArray;
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        auto* blockObj = new juce::DynamicObject();
        blockObj->setProperty("id", blockId);
        blockObj->setProperty("nodeId", static_cast<int>(nodeId.uid));

        auto posIt = blockPositions.find(blockId);
        if (posIt != blockPositions.end())
        {
            blockObj->setProperty("col", posIt->second.first);
            blockObj->setProperty("row", posIt->second.second);
        }

        // Look up block type and name from the graph node
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                blockObj->setProperty("type", stellarr::blockTypeToString(block->getBlockType()));
                blockObj->setProperty("name", block->getName());
            }
        }

        blocksArray.add(juce::var(blockObj));
    }

    juce::Array<juce::var> connectionsArray;
    // Build reverse map: nodeID → blockID
    std::map<juce::uint32, juce::String> nodeToBlock;
    for (auto& [blockId, nodeId] : blockNodeMap)
        nodeToBlock[nodeId.uid] = blockId;

    for (auto& conn : processor->getGraph().getConnections())
    {
        if (conn.source.channelIndex != 0) continue; // Only emit once per connection pair

        auto srcIt = nodeToBlock.find(conn.source.nodeID.uid);
        auto dstIt = nodeToBlock.find(conn.destination.nodeID.uid);
        if (srcIt == nodeToBlock.end() || dstIt == nodeToBlock.end()) continue;

        auto* connObj = new juce::DynamicObject();
        connObj->setProperty("sourceId", srcIt->second);
        connObj->setProperty("destId", dstIt->second);
        connectionsArray.add(juce::var(connObj));
    }

    auto* state = new juce::DynamicObject();
    state->setProperty("blocks", blocksArray);
    state->setProperty("connections", connectionsArray);
    emitToJs("graphState", state);
}
