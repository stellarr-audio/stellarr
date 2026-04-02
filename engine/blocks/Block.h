#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace stellarr
{

enum class BlockType
{
    input,
    output,
    gain, // test-only
    vst
};

inline juce::String blockTypeToString(BlockType type)
{
    switch (type)
    {
        case BlockType::input:   return "input";
        case BlockType::output:  return "output";
        case BlockType::gain:    return "gain";
        case BlockType::vst:     return "vst";
    }
    return "unknown";
}

inline BlockType blockTypeFromString(const juce::String& str)
{
    if (str == "input")   return BlockType::input;
    if (str == "output")  return BlockType::output;
    if (str == "gain")    return BlockType::gain;
    if (str == "vst")     return BlockType::vst;
    return BlockType::vst;
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
    virtual BlockType getBlockType() const = 0;

    int getNumAudioInputs() const  { return getTotalNumInputChannels(); }
    int getNumAudioOutputs() const { return getTotalNumOutputChannels(); }

    // Mix parameter (0.0 = fully dry, 1.0 = fully wet)
    float getMix() const { return mix.load(std::memory_order_relaxed); }

    void setMix(float value)
    {
        mix.store(juce::jlimit(0.0f, 1.0f, value), std::memory_order_relaxed);
    }

    // Reset transient state to defaults. Called when loading a preset.
    virtual void resetToDefault() {}

    // Derived blocks implement this instead of processBlock
    virtual void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) = 0;

    // Final: handles dry/wet blending, delegates to process()
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) final
    {
        auto mixVal = mix.load(std::memory_order_relaxed);

        if (!hasMix || mixVal >= 1.0f)
        {
            // Fully wet or mix not supported — skip dry buffer copy
            process(buffer, midi);
            return;
        }

        if (mixVal <= 0.0f)
        {
            // Fully dry — skip processing entirely
            return;
        }

        // Save dry signal
        if (dryBuffer.getNumChannels() != buffer.getNumChannels()
            || dryBuffer.getNumSamples() != buffer.getNumSamples())
        {
            dryBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
        }

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

        // Process (wet)
        process(buffer, midi);

        // Blend: output = dry * (1 - mix) + wet * mix
        float dryGain = 1.0f - mixVal;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                wet[i] = dry[i] * dryGain + wet[i] * mixVal;
        }
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        if (hasMix)
            dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock, false, false, true);

        prepareBlock(sampleRate, samplesPerBlock);
    }

    // Derived blocks override this for their prepare logic
    virtual void prepareBlock(double, int) {}

    virtual juce::var toJson() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", blockId.toString());
        obj->setProperty("type", blockTypeToString(getBlockType()));
        obj->setProperty("name", blockName);

        if (hasMix)
            obj->setProperty("mix", static_cast<double>(getMix()));

        return juce::var(obj);
    }

    virtual void fromJson(const juce::var& json)
    {
        if (auto* obj = json.getDynamicObject())
        {
            auto idStr = obj->getProperty("id").toString();
            if (idStr.isNotEmpty())
                blockId = juce::Uuid(idStr);

            if (hasMix && obj->hasProperty("mix"))
                setMix(static_cast<float>(obj->getProperty("mix")));
        }
    }

    // AudioProcessor boilerplate shared by all blocks
    const juce::String getName() const override { return blockName; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
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
    bool hasMix;
    std::atomic<float> mix { 1.0f };
    juce::AudioBuffer<float> dryBuffer;
};

} // namespace stellarr
