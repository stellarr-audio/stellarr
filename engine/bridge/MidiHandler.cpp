#include "../StellarrBridge.h"
#include "../StellarrProcessor.h"
#include "../blocks/Block.h"
#include "../blocks/PluginBlock.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"

void StellarrBridge::setupMidiMapper()
{
    if (processor == nullptr) return;

    auto& mapper = processor->getMidiMapper();

    mapper.onPresetChange = [this](int index) {
        juce::MessageManager::callAsync([this, index]() {
            auto json = juce::JSON::parse("{\"index\":" + juce::String(index) + "}");
            handleLoadPresetByIndex(json);
        });
    };

    mapper.onSceneSwitch = [this](int index) {
        juce::MessageManager::callAsync([this, index]() {
            auto json = juce::JSON::parse("{\"index\":" + juce::String(index) + "}");
            handleRecallScene(json);
        });
    };

    mapper.onBlockBypass = [this](const juce::String& blockId, bool state) {
        juce::MessageManager::callAsync([this, blockId, state]() {
            auto* block = findBlock(blockId);
            if (block == nullptr) return;

            block->setBypassed(state);
            markDirtyAndEmit(blockId, block);

            auto* detail = new juce::DynamicObject();
            detail->setProperty("blockId", blockId);
            detail->setProperty("bypassed", state);
            emitToJs("blockBypassChanged", detail);
        });
    };

    mapper.onBlockMix = [this](const juce::String& blockId, float value) {
        juce::MessageManager::callAsync([this, blockId, value]() {
            auto* block = findBlock(blockId);
            if (block == nullptr) return;

            block->setMix(value);
            markDirtyAndEmit(blockId, block);

            auto* detail = new juce::DynamicObject();
            detail->setProperty("blockId", blockId);
            detail->setProperty("mix", static_cast<double>(value));
            emitToJs("blockMixChanged", detail);
        });
    };

    mapper.onBlockBalance = [this](const juce::String& blockId, float value) {
        juce::MessageManager::callAsync([this, blockId, value]() {
            auto* block = findBlock(blockId);
            if (block == nullptr) return;

            block->setBalance(value);
            markDirtyAndEmit(blockId, block);

            auto* detail = new juce::DynamicObject();
            detail->setProperty("blockId", blockId);
            detail->setProperty("balance", static_cast<double>(value));
            emitToJs("blockBalanceChanged", detail);
        });
    };

    mapper.onBlockLevel = [this](const juce::String& blockId, float levelDb) {
        juce::MessageManager::callAsync([this, blockId, levelDb]() {
            auto* block = findBlock(blockId);
            if (block == nullptr) return;

            block->setLevelDb(levelDb);
            markDirtyAndEmit(blockId, block);

            auto* detail = new juce::DynamicObject();
            detail->setProperty("blockId", blockId);
            detail->setProperty("level", static_cast<double>(levelDb));
            emitToJs("blockLevelChanged", detail);
        });
    };

    mapper.onTunerToggle = [this](bool enabled) {
        juce::MessageManager::callAsync([this, enabled]() {
            tunerActive = enabled;
            for (auto& [blockId, nodeId] : blockNodeMap)
            {
                if (auto* node = processor->getGraph().getNodeForId(nodeId))
                {
                    if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
                        inputBlock->setTunerEnabled(enabled);
                    if (auto* outputBlock = dynamic_cast<stellarr::OutputBlock*>(node->getProcessor()))
                        outputBlock->setTunerMute(enabled);
                }
            }
        });
    };

    mapper.onLearnComplete = [this](int channel, int cc) {
        juce::MessageManager::callAsync([this, channel, cc]() {
            auto* detail = new juce::DynamicObject();
            detail->setProperty("channel", channel);
            detail->setProperty("cc", cc);
            emitToJs("midiLearnComplete", detail);
            emitMidiMappings();
        });
    };
}

void StellarrBridge::emitMidiMappings()
{
    if (processor == nullptr) return;

    auto& mapper = processor->getMidiMapper();
    auto* detail = new juce::DynamicObject();

    juce::Array<juce::var> arr;
    for (int i = 0; i < mapper.getNumMappings(); ++i)
    {
        auto& m = mapper.getMapping(i);
        auto* obj = new juce::DynamicObject();
        obj->setProperty("channel", m.channel);
        obj->setProperty("cc", m.ccNumber);
        obj->setProperty("target", MidiMapper::targetToString(m.target));
        if (m.blockId.isNotEmpty())
            obj->setProperty("blockId", m.blockId);
        arr.add(juce::var(obj));
    }

    detail->setProperty("mappings", arr);
    detail->setProperty("learning", mapper.isLearning());
    emitToJs("midiMappingsChanged", detail);
}
