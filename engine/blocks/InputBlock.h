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
        samplesPerBeat = sampleRate * 60.0 / bpm;
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        if (!testToneEnabled.load(std::memory_order_relaxed))
            return;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // Advance the sequence position
            auto noteLength = melody[melodyIndex].beats * samplesPerBeat;
            if (sampleInNote >= noteLength)
            {
                sampleInNote = 0.0;
                melodyIndex = (melodyIndex + 1) % melodySize;
                phase = 0.0;
            }

            auto freq = melody[melodyIndex].frequency;
            float sample = 0.0f;

            if (freq > 0.0)
            {
                // Amplitude envelope: quick attack, gentle decay over the note
                auto noteProgress = sampleInNote / noteLength;
                auto envelope = static_cast<float>(
                    noteProgress < 0.02 ? noteProgress / 0.02       // attack
                    : 1.0 - noteProgress * 0.4);                    // decay
                if (envelope < 0.0f) envelope = 0.0f;

                sample = envelope * amplitude * static_cast<float>(
                    std::sin(phase * juce::MathConstants<double>::twoPi));

                phase += freq / currentSampleRate;
                if (phase >= 1.0) phase -= 1.0;
            }

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, sample);

            sampleInNote += 1.0;
        }
    }

    void resetToDefault() override
    {
        setTestToneEnabled(false);
    }

    void setTestToneEnabled(bool enabled)
    {
        testToneEnabled.store(enabled, std::memory_order_relaxed);
        if (!enabled)
        {
            phase = 0.0;
            melodyIndex = 0;
            sampleInNote = 0.0;
        }
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
    struct Note { double frequency; double beats; };

    // A short pentatonic riff in E minor — loops naturally
    static constexpr int melodySize = 12;
    static constexpr Note melody[melodySize] = {
        {329.63, 0.5},   // E4
        {392.00, 0.5},   // G4
        {440.00, 0.5},   // A4
        {493.88, 1.0},   // B4
        {440.00, 0.5},   // A4
        {392.00, 0.5},   // G4
        {329.63, 1.0},   // E4
        {0.0,    0.5},   // rest
        {293.66, 0.5},   // D4
        {329.63, 0.5},   // E4
        {392.00, 1.0},   // G4
        {0.0,    1.0},   // rest
    };

    static constexpr double bpm = 120.0;
    static constexpr float amplitude = 0.45f;

    std::atomic<bool> testToneEnabled { false };
    double phase = 0.0;
    double currentSampleRate = 44100.0;
    double samplesPerBeat = 44100.0 * 60.0 / 120.0;
    int melodyIndex = 0;
    double sampleInNote = 0.0;
};

} // namespace stellarr
