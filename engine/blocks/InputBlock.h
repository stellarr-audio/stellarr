#pragma once
#include "Block.h"
#include <atomic>

namespace stellarr
{

class InputBlock final : public Block
{
public:
    InputBlock() : Block("Input", 2, 2) {}

    BlockType getBlockType() const override { return BlockType::input; }

    void prepareToPlay(double sampleRate, int) override
    {
        currentSampleRate = sampleRate;
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        if (!testToneEnabled.load(std::memory_order_relaxed))
            return;

        constexpr double frequency = 440.0;
        constexpr float amplitude = 0.5f;
        auto phaseIncrement = frequency / currentSampleRate;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            auto sample = amplitude * static_cast<float>(
                std::sin(phase * juce::MathConstants<double>::twoPi));

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, sample);

            phase += phaseIncrement;
            if (phase >= 1.0) phase -= 1.0;
        }
    }

    void setTestToneEnabled(bool enabled)
    {
        testToneEnabled.store(enabled, std::memory_order_relaxed);
        if (!enabled) phase = 0.0;
    }

    bool isTestToneEnabled() const
    {
        return testToneEnabled.load(std::memory_order_relaxed);
    }

    juce::var toJson() const override
    {
        auto json = Block::toJson();
        if (auto* obj = json.getDynamicObject())
            obj->setProperty("testTone", isTestToneEnabled());
        return json;
    }

    void fromJson(const juce::var& json) override
    {
        Block::fromJson(json);
        if (auto* obj = json.getDynamicObject())
            setTestToneEnabled(static_cast<bool>(obj->getProperty("testTone")));
    }

private:
    std::atomic<bool> testToneEnabled { false };
    double phase = 0.0;
    double currentSampleRate = 44100.0;
};

} // namespace stellarr
