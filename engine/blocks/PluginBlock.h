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

class PluginBlock final : public Block, private juce::Timer
{
public:
    PluginBlock() : Block("Plugin", 2, 2)
    {
        states.push_back({});
    }

    ~PluginBlock() override { stopTimer(); }

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
        if (!pluginReady.load(std::memory_order_acquire))
            return;

        juce::SpinLock::ScopedTryLockType lock(pluginLock);

        if (lock.isLocked() && plugin != nullptr
            && plugin->getTotalNumInputChannels() <= buffer.getNumChannels()
            && plugin->getTotalNumOutputChannels() <= buffer.getNumChannels())
        {
            plugin->processBlock(buffer, midi);
        }
    }

    /**
     * Swap a new plugin instance into this block.
     *
     * If readyDelayMs > 0, the block stays in "not ready" state for that
     * many milliseconds after the swap, so the audio thread treats the
     * block as pass-through while the plugin finishes any background
     * initialisation (NAM, IR loaders, ML amp sims). With readyDelayMs = 0
     * (the default), the block is marked ready immediately, matching the
     * behaviour required by preset and paste paths that prepare under a
     * graph-level suspension.
     */
    void setPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin,
                   const juce::String& identifier,
                   int readyDelayMs = 0);

    /// Suggested delay for the live plugin-picker swap path.
    static constexpr int pluginPickReadyDelayMs = 3000;

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
    void applyState(const PluginBlockState& s, bool suspend = true);

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
    // Timer callback: mark the plugin ready after its deferred-readiness
    // window elapses. Started by setPlugin() when readyDelayMs > 0.
    void timerCallback() override
    {
        stopTimer();
        pluginReady.store(true, std::memory_order_release);
    }

    std::atomic<bool> pluginReady { false };
    mutable juce::SpinLock pluginLock;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::unique_ptr<PluginWindow> pluginWindow;
    juce::String pluginIdentifier;
    juce::String pluginStateBase64;
    juce::String missingPluginName;
    bool pluginMissing = false;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    mutable std::vector<PluginBlockState> states;
    mutable int activeStateIndex = 0;
    std::set<int> dirtyStates;
};

} // namespace stellarr
