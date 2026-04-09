#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace stellarr
{

enum class BlockType
{
    input,
    output,
    gain, // test-only
    plugin
};

enum class BypassMode
{
    thru,
    muteIn,
    muteOut,
    mute
};

inline juce::String bypassModeToString(BypassMode mode)
{
    switch (mode)
    {
        case BypassMode::thru:    return "thru";
        case BypassMode::muteIn:  return "muteIn";
        case BypassMode::muteOut: return "muteOut";
        case BypassMode::mute:    return "mute";
    }
    return "thru";
}

inline BypassMode bypassModeFromString(const juce::String& str)
{
    if (str == "muteIn")  return BypassMode::muteIn;
    if (str == "muteOut") return BypassMode::muteOut;
    if (str == "mute")    return BypassMode::mute;
    return BypassMode::thru;
}

inline juce::String blockTypeToString(BlockType type)
{
    switch (type)
    {
        case BlockType::input:   return "input";
        case BlockType::output:  return "output";
        case BlockType::gain:    return "gain";
        case BlockType::plugin:   return "plugin";
    }
    return "unknown";
}

inline BlockType blockTypeFromString(const juce::String& str)
{
    if (str == "input")   return BlockType::input;
    if (str == "output")  return BlockType::output;
    if (str == "gain")    return BlockType::gain;
    if (str == "plugin" || str == "vst")  return BlockType::plugin;
    return BlockType::plugin;
}

class Block : public juce::AudioProcessor
{
public:
    Block(const juce::String& name, int numInputChannels, int numOutputChannels,
          bool supportsMix = true)
        : AudioProcessor(makeBuses(numInputChannels, numOutputChannels)),
          blockId(juce::Uuid()),
          blockName(name),
          hasMix(supportsMix)
    {
    }

    ~Block() override = default;

    const juce::Uuid& getBlockId() const { return blockId; }
    void regenerateBlockId() { blockId = juce::Uuid(); }
    virtual BlockType getBlockType() const = 0;

    int getNumAudioInputs() const  { return getTotalNumInputChannels(); }
    int getNumAudioOutputs() const { return getTotalNumOutputChannels(); }

    // User-editable display name (empty = use default blockName)
    juce::String getDisplayName() const { return displayName; }
    void setDisplayName(const juce::String& name) { displayName = name; }

    // User-chosen block color (empty = use default type color)
    juce::String getBlockColor() const { return blockColor; }
    void setBlockColor(const juce::String& color) { blockColor = color; }

    // Mix parameter (0.0 = fully dry, 1.0 = fully wet)
    float getMix() const { return mix.load(std::memory_order_relaxed); }

    void setMix(float value)
    {
        mix.store(juce::jlimit(0.0f, 1.0f, value), std::memory_order_relaxed);
    }

    // Balance: -1.0 (full left) to +1.0 (full right), 0.0 = centre
    float getBalance() const { return balance.load(std::memory_order_relaxed); }

    void setBalance(float value)
    {
        balance.store(juce::jlimit(-1.0f, 1.0f, value), std::memory_order_relaxed);
    }

    // Level: output gain in linear scale. 1.0 = 0 dB (unity), 0.0 = -inf dB (mute)
    float getLevel() const { return level.load(std::memory_order_relaxed); }

    void setLevel(float linearGain)
    {
        level.store(juce::jlimit(0.0f, 3.981f, linearGain), std::memory_order_relaxed); // max ~+12 dB
    }

    // dB helpers for UI/serialization
    float getLevelDb() const
    {
        auto g = getLevel();
        if (g <= 0.001f) return -60.0f;
        return 20.0f * std::log10(g);
    }

    void setLevelDb(float db)
    {
        if (db <= -60.0f) setLevel(0.0f);
        else setLevel(std::pow(10.0f, db / 20.0f));
    }

    // Bypass: when true, block is disengaged (THRU mode — audio passes through unchanged)
    bool isBypassed() const { return bypassed.load(std::memory_order_relaxed); }

    void setBypassed(bool value)
    {
        bypassed.store(value, std::memory_order_relaxed);
    }

    BypassMode getBypassMode() const
    {
        return static_cast<BypassMode>(bypassMode.load(std::memory_order_relaxed));
    }

    void setBypassMode(BypassMode mode)
    {
        bypassMode.store(static_cast<int>(mode), std::memory_order_relaxed);
    }

    // Per-block peak level metering (read from message thread, written from audio thread)
    float getPeakLevel() const
    {
        return peakLevel.exchange(0.0f, std::memory_order_relaxed);
    }

    // Reset transient state to defaults. Called when loading a preset.
    virtual void resetToDefault() {}

    // Derived blocks implement this instead of processBlock
    virtual void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) = 0;

    // Final: handles bypass, dry/wet blending, balance, and level
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) final;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    // Derived blocks override this for their prepare logic
    virtual void prepareBlock(double, int) {}

    virtual juce::var toJson() const;
    virtual void fromJson(const juce::var& json);

    // AudioProcessor boilerplate shared by all blocks
    const juce::String getName() const override { return blockName; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    void releaseResources() override {}

private:
    static BusesProperties makeBuses(int numIn, int numOut)
    {
        BusesProperties buses;
        if (numIn > 0)
            buses = buses.withInput("Input",
                numIn == 1 ? juce::AudioChannelSet::mono()
                           : juce::AudioChannelSet::stereo(), true);
        if (numOut > 0)
            buses = buses.withOutput("Output",
                numOut == 1 ? juce::AudioChannelSet::mono()
                            : juce::AudioChannelSet::stereo(), true);
        return buses;
    }

    juce::Uuid blockId;
    juce::String blockName;
    juce::String displayName;
    juce::String blockColor;
    bool hasMix;
    std::atomic<float> mix { 1.0f };
    std::atomic<float> balance { 0.0f };
    std::atomic<float> level { 1.0f };
    std::atomic<bool> bypassed { false };
    std::atomic<int> bypassMode { static_cast<int>(BypassMode::thru) };
    mutable std::atomic<float> peakLevel { 0.0f };
    juce::AudioBuffer<float> dryBuffer;
};

} // namespace stellarr
