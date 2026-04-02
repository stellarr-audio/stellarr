#pragma once
#include "Block.h"
#include "../PluginWindow.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace stellarr
{

class VstBlock final : public Block
{
public:
    VstBlock() : Block("VST", 2, 2) {}

    BlockType getBlockType() const override { return BlockType::vst; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr)
            plugin->prepareToPlay(sampleRate, samplesPerBlock);
    }

    void releaseResources() override
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr)
            plugin->releaseResources();
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        juce::SpinLock::ScopedTryLockType lock(pluginLock);

        if (lock.isLocked() && plugin != nullptr)
            plugin->processBlock(buffer, midi);
    }

    void setPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin,
                   const juce::String& identifier)
    {
        if (newPlugin != nullptr)
            newPlugin->prepareToPlay(currentSampleRate, currentBlockSize);

        {
            juce::SpinLock::ScopedLockType lock(pluginLock);
            std::swap(plugin, newPlugin);
            pluginIdentifier = identifier;
        }

        // Old plugin (now in newPlugin) is released on the message thread
        if (newPlugin != nullptr)
            newPlugin->releaseResources();
    }

    juce::String getPluginIdentifier() const { return pluginIdentifier; }

    juce::String getPluginName() const
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr)
            return plugin->getName();
        return {};
    }

    bool hasPlugin() const
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        return plugin != nullptr;
    }

    void openPluginEditor()
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin == nullptr) return;

        if (pluginWindow != nullptr && pluginWindow->isVisible())
        {
            pluginWindow->toFront(true);
            return;
        }

        if (auto* editor = plugin->createEditorIfNecessary())
            pluginWindow = std::make_unique<PluginWindow>(editor, plugin->getName());
    }

    void closePluginEditor()
    {
        pluginWindow = nullptr;
    }

    juce::var toJson() const override
    {
        auto json = Block::toJson();
        if (auto* obj = json.getDynamicObject())
        {
            obj->setProperty("pluginId", pluginIdentifier);
            obj->setProperty("pluginName", getPluginName());
        }
        return json;
    }

    void fromJson(const juce::var& json) override
    {
        Block::fromJson(json);
        if (auto* obj = json.getDynamicObject())
            pluginIdentifier = obj->getProperty("pluginId").toString();
    }

private:
    mutable juce::SpinLock pluginLock;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::unique_ptr<PluginWindow> pluginWindow;
    juce::String pluginIdentifier;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
};

} // namespace stellarr
