#pragma once
#include "Block.h"

namespace stellarr
{

// Test-only block. Applies a simple gain stage for verifying audio routing
// in automated tests. Not exposed in the UI or palette — end users add gain
// via FX blocks with a gain/EQ effect type.
class GainBlock final : public Block
{
public:
    GainBlock()
        : Block("Gain", 2, 2),
          gainParam(new juce::AudioParameterFloat(
              juce::ParameterID{"gain", 1}, "Gain", 0.0f, 2.0f, 1.0f))
    {
        addParameter(gainParam);
    }

    BlockType getBlockType() const override { return BlockType::gain; }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        auto gain = gainParam->get();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, buffer.getNumSamples(), gain);
    }

    juce::var toJson() const override
    {
        auto json = Block::toJson();

        if (auto* obj = json.getDynamicObject())
            obj->setProperty("gain", static_cast<double>(gainParam->get()));

        return json;
    }

    void fromJson(const juce::var& json) override
    {
        Block::fromJson(json);

        if (auto* obj = json.getDynamicObject())
            *gainParam = static_cast<float>(obj->getProperty("gain"));
    }

    void setGain(float value) { *gainParam = value; }
    float getGain() const { return gainParam->get(); }

private:
    juce::AudioParameterFloat* gainParam;
};

} // namespace stellarr
