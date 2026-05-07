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
    InputBlock() : Block("Input", 2, 2, false)
    {
        setMeasureLoudness(true); // Input always measures (footer IN meter)
    }

    BlockType getBlockType() const override { return BlockType::input; }

    void prepareBlock(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

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

    bool isTestToneEnabled() const { return testToneEnabled.load(std::memory_order_relaxed); }
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

    void setReferencePitch(float hz) { referencePitch.store(hz, std::memory_order_relaxed); }
    float getReferencePitch() const { return referencePitch.load(std::memory_order_relaxed); }

    // Test tone state is intentionally NOT persisted in toJson / fromJson.
    // Test tone is a transient dev utility (gated by developer mode); a saved
    // session that captured it as ON would resume the tone after restore even
    // when developer mode is OFF — leaving an audible tone with no UI control
    // to stop it. Restored InputBlocks always start with test tone disabled.

private:
    void analyzeTunerBuffer(const juce::AudioBuffer<float>& buffer);
    void runYinDetection();

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
    std::atomic<float> referencePitch { 440.0f };

    std::vector<float> tunerBuffer;
    std::vector<float> yinBuffer;
    std::vector<float> linearBuffer;
    int tunerBufferWritePos = 0;
    int tunerSamplesAccumulated = 0;
};

} // namespace stellarr
