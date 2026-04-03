#pragma once
#include "Block.h"
#include <atomic>
#include <vector>
#include <cmath>

namespace stellarr
{

class InputBlock final : public Block
{
public:
    InputBlock() : Block("Input", 2, 2, false) {}

    BlockType getBlockType() const override { return BlockType::input; }

    void prepareBlock(double sampleRate, int) override
    {
        currentSampleRate = sampleRate;
        samplesPerBeat = sampleRate * 60.0 / bpm;

        // Tuner buffers
        tunerBuffer.resize(tunerBufferSize, 0.0f);
        yinBuffer.resize(tunerBufferSize / 2, 0.0f);
        tunerBufferWritePos = 0;
        tunerSamplesAccumulated = 0;
    }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // Test tone generation
        if (!testToneEnabled.load(std::memory_order_relaxed))
        {
            // Tuner analysis on real input (no test tone)
            analyzeTunerBuffer(buffer);
            return;
        }

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
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
                auto noteProgress = sampleInNote / noteLength;
                auto envelope = static_cast<float>(
                    noteProgress < 0.02 ? noteProgress / 0.02
                    : 1.0 - noteProgress * 0.4);
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

        // Tuner analysis on test tone output
        analyzeTunerBuffer(buffer);
    }

    void resetToDefault() override
    {
        setTestToneEnabled(false);
        setTunerEnabled(false);
    }

    // Test tone
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

    // Tuner
    void setTunerEnabled(bool enabled)
    {
        tunerEnabled.store(enabled, std::memory_order_relaxed);
        if (!enabled)
        {
            detectedNoteIndex.store(-1, std::memory_order_relaxed);
            detectedFrequency.store(0.0f, std::memory_order_relaxed);
            detectedCents.store(0.0f, std::memory_order_relaxed);
            detectedOctave.store(-1, std::memory_order_relaxed);
            detectedConfidence.store(0.0f, std::memory_order_relaxed);
        }
    }

    bool isTunerEnabled() const { return tunerEnabled.load(std::memory_order_relaxed); }
    float getTunerFrequency() const { return detectedFrequency.load(std::memory_order_relaxed); }
    float getTunerCents() const { return detectedCents.load(std::memory_order_relaxed); }
    int getTunerNoteIndex() const { return detectedNoteIndex.load(std::memory_order_relaxed); }
    int getTunerOctave() const { return detectedOctave.load(std::memory_order_relaxed); }
    float getTunerConfidence() const { return detectedConfidence.load(std::memory_order_relaxed); }

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
    void analyzeTunerBuffer(const juce::AudioBuffer<float>& buffer)
    {
        if (!tunerEnabled.load(std::memory_order_relaxed) || tunerBuffer.empty())
            return;

        const float* input = buffer.getReadPointer(0);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            tunerBuffer[tunerBufferWritePos] = input[i];
            tunerBufferWritePos = (tunerBufferWritePos + 1) % tunerBufferSize;
        }
        tunerSamplesAccumulated += buffer.getNumSamples();

        if (tunerSamplesAccumulated >= tunerBufferSize)
        {
            tunerSamplesAccumulated = 0;
            runYinDetection();
        }
    }

    // -- YIN pitch detection --------------------------------------------------

    void runYinDetection()
    {
        auto halfSize = tunerBufferSize / 2;

        // Linearize circular buffer
        std::vector<float> linear(tunerBufferSize);
        for (int i = 0; i < tunerBufferSize; ++i)
            linear[i] = tunerBuffer[(tunerBufferWritePos + i) % tunerBufferSize];

        // Step 1 & 2: Difference function + cumulative mean normalized
        yinBuffer[0] = 1.0f;
        float runningSum = 0.0f;

        for (int tau = 1; tau < halfSize; ++tau)
        {
            float sum = 0.0f;
            for (int j = 0; j < halfSize; ++j)
            {
                float delta = linear[j] - linear[j + tau];
                sum += delta * delta;
            }

            runningSum += sum;
            yinBuffer[tau] = (runningSum > 0.0f)
                ? sum * static_cast<float>(tau) / runningSum
                : 1.0f;
        }

        // Step 3: Absolute threshold -- find first dip below threshold
        constexpr float threshold = 0.15f;
        int tauEstimate = -1;

        for (int tau = 2; tau < halfSize; ++tau)
        {
            if (yinBuffer[tau] < threshold)
            {
                // Find the local minimum
                while (tau + 1 < halfSize && yinBuffer[tau + 1] < yinBuffer[tau])
                    ++tau;
                tauEstimate = tau;
                break;
            }
        }

        if (tauEstimate < 0)
        {
            // No pitch detected
            detectedNoteIndex.store(-1, std::memory_order_relaxed);
            detectedConfidence.store(0.0f, std::memory_order_relaxed);
            return;
        }

        // Step 4: Parabolic interpolation
        float betterTau = static_cast<float>(tauEstimate);
        if (tauEstimate > 0 && tauEstimate < halfSize - 1)
        {
            float s0 = yinBuffer[tauEstimate - 1];
            float s1 = yinBuffer[tauEstimate];
            float s2 = yinBuffer[tauEstimate + 1];
            float adjustment = (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));
            if (std::isfinite(adjustment))
                betterTau = static_cast<float>(tauEstimate) + adjustment;
        }

        float freq = static_cast<float>(currentSampleRate) / betterTau;

        // Sanity check: guitar range ~30 Hz to ~2000 Hz
        if (freq < 30.0f || freq > 2000.0f)
        {
            detectedNoteIndex.store(-1, std::memory_order_relaxed);
            detectedConfidence.store(0.0f, std::memory_order_relaxed);
            return;
        }

        // Convert to note
        float midiNote = 12.0f * std::log2(freq / 440.0f) + 69.0f;
        int roundedNote = static_cast<int>(std::round(midiNote));
        float cents = (midiNote - static_cast<float>(roundedNote)) * 100.0f;
        int noteIndex = ((roundedNote % 12) + 12) % 12; // ensure positive
        int octave = roundedNote / 12 - 1;

        float confidence = 1.0f - yinBuffer[tauEstimate];
        if (confidence < 0.0f) confidence = 0.0f;
        if (confidence > 1.0f) confidence = 1.0f;

        detectedFrequency.store(freq, std::memory_order_relaxed);
        detectedCents.store(cents, std::memory_order_relaxed);
        detectedNoteIndex.store(noteIndex, std::memory_order_relaxed);
        detectedOctave.store(octave, std::memory_order_relaxed);
        detectedConfidence.store(confidence, std::memory_order_relaxed);
    }

    // -- Test tone data -------------------------------------------------------

    struct Note { double frequency; double beats; };

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

    // -- Tuner data -----------------------------------------------------------

    static constexpr int tunerBufferSize = 2048;

    std::atomic<bool> tunerEnabled { false };
    std::atomic<float> detectedFrequency { 0.0f };
    std::atomic<float> detectedCents { 0.0f };
    std::atomic<int> detectedNoteIndex { -1 };  // 0=C, 1=C#, ... 11=B
    std::atomic<int> detectedOctave { -1 };
    std::atomic<float> detectedConfidence { 0.0f };

    std::vector<float> tunerBuffer;
    std::vector<float> yinBuffer;
    int tunerBufferWritePos = 0;
    int tunerSamplesAccumulated = 0;
};

} // namespace stellarr
