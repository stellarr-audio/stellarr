#pragma once
#include "Block.h"
#include "../PluginWindow.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <set>

namespace stellarr
{

struct PluginBlockState
{
    juce::String pluginStateBase64;
    float mix = 1.0f;
    float balance = 0.0f;
    float levelDb = 0.0f;
    bool bypassed = false;
    BypassMode bypassMode = BypassMode::thru;
};

class PluginBlock final : public Block
{
public:
    PluginBlock() : Block("Plugin", 2, 2)
    {
        states.push_back({});
    }

    BlockType getBlockType() const override { return BlockType::plugin; }

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
        // Skip processing during warmup to let plugins finish background init
        auto remaining = warmupSamplesRemaining.load(std::memory_order_relaxed);
        if (remaining > 0)
        {
            warmupSamplesRemaining.store(
                std::max(0, remaining - buffer.getNumSamples()),
                std::memory_order_relaxed);
            return;
        }

        juce::SpinLock::ScopedTryLockType lock(pluginLock);

        if (lock.isLocked() && plugin != nullptr
            && plugin->getTotalNumInputChannels() <= buffer.getNumChannels()
            && plugin->getTotalNumOutputChannels() <= buffer.getNumChannels())
        {
            plugin->processBlock(buffer, midi);
        }
    }

    void setPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin,
                   const juce::String& identifier);

    juce::String getPluginIdentifier() const { return pluginIdentifier; }

    juce::String getPluginName() const
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr) return plugin->getName();
        return missingPluginName;
    }

    bool isPluginMissing() const { return pluginMissing; }
    void setPluginMissing(bool missing) { pluginMissing = missing; }
    void setMissingPluginName(const juce::String& name) { missingPluginName = name; }

    juce::String getPluginFormat() const
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        return plugin != nullptr ? plugin->getPluginDescription().pluginFormatName : juce::String{};
    }

    bool hasPlugin() const
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        return plugin != nullptr;
    }

    void openPluginEditor();
    void closePluginEditor() { pluginWindow = nullptr; }

    // -- State management -----------------------------------------------------

    static constexpr int maxStates = 16;

    int getNumStates() const { return static_cast<int>(states.size()); }
    int getActiveStateIndex() const { return activeStateIndex; }
    const std::set<int>& getDirtyStates() const { return dirtyStates; }
    void markDirty() { dirtyStates.insert(activeStateIndex); }

    PluginBlockState captureCurrentState() const;
    void applyState(const PluginBlockState& s);

    void saveCurrentState()
    {
        if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
            states[static_cast<size_t>(activeStateIndex)] = captureCurrentState();
        dirtyStates.clear();
    }

    bool addState();
    bool recallState(int index);
    bool deleteState(int index);

    // -- Serialization --------------------------------------------------------

    juce::var toJson() const override;
    void fromJson(const juce::var& json) override;
    void restorePluginState();

private:
    mutable juce::SpinLock pluginLock;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::unique_ptr<PluginWindow> pluginWindow;
    juce::String pluginIdentifier;
    juce::String pluginStateBase64;
    juce::String missingPluginName;
    bool pluginMissing = false;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    std::atomic<int> warmupSamplesRemaining { 0 };
    static constexpr double warmupSeconds = 1.0;

    // Brief mute after plugin load/state change to let background init finish
    void startWarmup()
    {
        warmupSamplesRemaining.store(
            static_cast<int>(currentSampleRate * warmupSeconds),
            std::memory_order_relaxed);
    }

    std::vector<PluginBlockState> states;
    int activeStateIndex = 0;
    std::set<int> dirtyStates;
};

} // namespace stellarr
