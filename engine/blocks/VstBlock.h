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

    void prepareBlock(double sampleRate, int samplesPerBlock) override
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr)
            plugin->prepareToPlay(sampleRate, samplesPerBlock);
    }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        juce::SpinLock::ScopedTryLockType lock(pluginLock);

        if (lock.isLocked() && plugin != nullptr)
            plugin->processBlock(buffer, midi);
    }

    void setPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin,
                   const juce::String& identifier)
    {
        // Close the editor window before swapping — the old plugin's UI
        // references the instance and will crash if it tries to draw
        // after the instance is destroyed.
        pluginWindow = nullptr;

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
        juce::AudioPluginInstance* p = nullptr;
        juce::String name;

        {
            juce::SpinLock::ScopedLockType lock(pluginLock);
            if (plugin == nullptr) return;
            p = plugin.get();
            name = plugin->getName();
        }

        if (pluginWindow != nullptr && pluginWindow->isVisible())
        {
            pluginWindow->toFront(true);
            return;
        }

        // AU plugin editors on macOS 26 crash in CoreAudioAUUI during layout
        // with an Objective-C exception that C++ try/catch cannot intercept.
        // Use the generic parameter editor for AU plugins, native editor for VST3.
        auto format = p->getPluginDescription().pluginFormatName;

        if (format == "VST3")
        {
            if (auto* editor = p->createEditorAndMakeActive())
                pluginWindow = std::make_unique<PluginWindow>(editor, name);
        }
        else
        {
            auto* generic = new juce::GenericAudioProcessorEditor(*p);
            pluginWindow = std::make_unique<PluginWindow>(generic, name);
        }
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

            juce::SpinLock::ScopedLockType lock(pluginLock);
            if (plugin != nullptr)
            {
                juce::MemoryBlock state;
                plugin->getStateInformation(state);
                obj->setProperty("pluginState", state.toBase64Encoding());
            }
        }
        return json;
    }

    void fromJson(const juce::var& json) override
    {
        Block::fromJson(json);
        if (auto* obj = json.getDynamicObject())
        {
            pluginIdentifier = obj->getProperty("pluginId").toString();
            pluginStateBase64 = obj->getProperty("pluginState").toString();
        }
    }

    // Call after the plugin instance has been loaded to restore its state
    void restorePluginState()
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr && pluginStateBase64.isNotEmpty())
        {
            juce::MemoryBlock state;
            state.fromBase64Encoding(pluginStateBase64);
            plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            pluginStateBase64.clear();
        }
    }

private:
    mutable juce::SpinLock pluginLock;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::unique_ptr<PluginWindow> pluginWindow;
    juce::String pluginIdentifier;
    juce::String pluginStateBase64;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
};

} // namespace stellarr
