#pragma once
#include "Block.h"
#include "../utils/ToneGenerator.h"
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
        toneGenerator.prepareToPlay(sampleRate);

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
            analyzeTunerBuffer(buffer);
            return;
        }

        toneGenerator.fillBuffer(buffer);
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
            toneGenerator.reset();
    }

    bool isTestToneEnabled() const
    {
        return testToneEnabled.load(std::memory_order_relaxed);
    }

    bool loadTestToneSample(const juce::File& file) { return toneGenerator.loadSample(file); }
    void clearTestToneSample() { toneGenerator.clearSample(); }
    bool isUsingSample() const { return toneGenerator.isUsingSample(); }
    juce::String getCurrentSampleName() const { return toneGenerator.getCurrentSampleName(); }

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
            tunerBuffer[static_cast<size_t>(tunerBufferWritePos)] = input[i];
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
        const size_t bufSize = static_cast<size_t>(tunerBufferSize);
        const size_t halfSize = bufSize / 2;

        // Linearize circular buffer (reuses member to avoid allocation)
        if (linearBuffer.size() != bufSize)
            linearBuffer.resize(bufSize);

        for (size_t i = 0; i < bufSize; ++i)
            linearBuffer[i] = tunerBuffer[(static_cast<size_t>(tunerBufferWritePos) + i) % bufSize];

        // Step 1 & 2: Difference function + cumulative mean normalized
        yinBuffer[0] = 1.0f;
        float runningSum = 0.0f;

        for (size_t tau = 1; tau < halfSize; ++tau)
        {
            float sum = 0.0f;
            for (size_t j = 0; j < halfSize && j + tau < bufSize; ++j)
            {
                float delta = linearBuffer[j] - linearBuffer[j + tau];
                sum += delta * delta;
            }

            runningSum += sum;
            yinBuffer[tau] = (runningSum > 0.0f)
                ? sum * static_cast<float>(tau) / runningSum
                : 1.0f;
        }

        // Step 3: Absolute threshold — find first dip below threshold
        constexpr float threshold = 0.15f;
        size_t tauEstimate = 0;
        bool found = false;

        for (size_t tau = 2; tau < halfSize; ++tau)
        {
            if (yinBuffer[tau] < threshold)
            {
                while (tau + 1 < halfSize && yinBuffer[tau + 1] < yinBuffer[tau])
                    ++tau;
                tauEstimate = tau;
                found = true;
                break;
            }
        }

        if (!found)
        {
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

    // -- Members --------------------------------------------------------------

    ToneGenerator toneGenerator;
    std::atomic<bool> testToneEnabled { false };
    double currentSampleRate = 44100.0;

    // Tuner
    static constexpr int tunerBufferSize = 2048;
    std::atomic<bool> tunerEnabled { false };
    std::atomic<float> detectedFrequency { 0.0f };
    std::atomic<float> detectedCents { 0.0f };
    std::atomic<int> detectedNoteIndex { -1 };
    std::atomic<int> detectedOctave { -1 };
    std::atomic<float> detectedConfidence { 0.0f };

    std::vector<float> tunerBuffer;
    std::vector<float> yinBuffer;
    std::vector<float> linearBuffer;
    int tunerBufferWritePos = 0;
    int tunerSamplesAccumulated = 0;
};

} // namespace stellarr
