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
    Block(const juce::String& name, int numInputChannels, int numOutputChannels)
        : AudioProcessor(makeBuses(numInputChannels, numOutputChannels)),
          blockId(juce::Uuid()),
          blockName(name)
    {
    }

    ~Block() override = default;

    const juce::Uuid& getBlockId() const { return blockId; }
    virtual BlockType getBlockType() const = 0;

    int getNumAudioInputs() const  { return getTotalNumInputChannels(); }
    int getNumAudioOutputs() const { return getTotalNumOutputChannels(); }

    virtual juce::var toJson() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", blockId.toString());
        obj->setProperty("type", blockTypeToString(getBlockType()));
        obj->setProperty("name", blockName);
        return juce::var(obj);
    }

    virtual void fromJson(const juce::var& json)
    {
        if (auto* obj = json.getDynamicObject())
        {
            auto idStr = obj->getProperty("id").toString();
            if (idStr.isNotEmpty())
                blockId = juce::Uuid(idStr);
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
};

} // namespace stellarr
