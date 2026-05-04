#include "../StellarrBridge.h"
#include "../StellarrProcessor.h"
#include "../StellarrPlatform.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"

// -- Test tone + tuner handlers ----------------------------------------------

void StellarrBridge::handleToggleTestTone(const juce::var& json)
{
    if (processor == nullptr) return;
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
            emit("testToneChanged", detail);
        }
    }
}

void StellarrBridge::handleGetTestToneSamples()
{
    auto samplesDir = stellarrGetBundleResource("samples");
    juce::Array<juce::var> files;

    if (samplesDir.isDirectory())
    {
        for (auto& f : samplesDir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        {
            auto name = f.getFileNameWithoutExtension();
            if (name.equalsIgnoreCase("placeholder")) continue; // skip submodule placeholder file
            files.add(juce::var(name));
        }
    }

    // Add "Synth (Default)" as first option
    juce::Array<juce::var> sorted;
    sorted.add(juce::var("Synth (Default)"));
    for (auto& f : files)
        sorted.add(f);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("samples", sorted);
    emit("testToneSamplesUpdated", detail);
}

void StellarrBridge::handleSetTestToneSample(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto sampleName = obj->getProperty("sample").toString();
    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return;

    if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
    {
        if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
        {
            if (sampleName == "Synth (Default)" || sampleName.isEmpty())
            {
                inputBlock->clearTestToneSample();
            }
            else
            {
                auto samplesDir = stellarrGetBundleResource("samples");
                auto file = samplesDir.getChildFile(sampleName + ".wav");
                inputBlock->loadTestToneSample(file);
            }

            auto* detail = new juce::DynamicObject();
            detail->setProperty("blockId", blockId);
            detail->setProperty("sample", inputBlock->isUsingSample()
                ? inputBlock->getCurrentSampleName() : juce::String("Synth (Default)"));
            emit("testToneSampleChanged", detail);
        }
    }
}

void StellarrBridge::handleSetTunerEnabled(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    bool enabled = static_cast<bool>(obj->getProperty("enabled"));
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
}
