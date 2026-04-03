#include "TestUtils.h"
#include "blocks/InputBlock.h"

static bool testTunerDetects440()
{
    printf("Test: tuner detects 440 Hz as A4... ");

    stellarr::InputBlock input;
    input.prepareToPlay(kSampleRate, kBlockSize);
    input.setTunerEnabled(true);
    input.setTestToneEnabled(false);

    // Feed 440 Hz sine through process() in block-sized chunks
    // Need enough samples to fill the 2048 tuner buffer
    for (int pass = 0; pass < 5; ++pass)
    {
        juce::AudioBuffer<float> buffer(2, kBlockSize);
        for (int i = 0; i < kBlockSize; ++i)
        {
            float sample = std::sin(kTwoPi * 440.0f * static_cast<float>(pass * kBlockSize + i) / static_cast<float>(kSampleRate));
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        juce::MidiBuffer midi;
        input.processBlock(buffer, midi);
    }

    float freq = input.getTunerFrequency();
    int noteIdx = input.getTunerNoteIndex();
    float confidence = input.getTunerConfidence();

    if (confidence < 0.3f)
    {
        fprintf(stderr, "  low confidence: %f\n", static_cast<double>(confidence));
        printf("FAIL\n");
        return false;
    }

    if (std::abs(freq - 440.0f) > 2.0f)
    {
        fprintf(stderr, "  frequency off: expected ~440, got %f\n", static_cast<double>(freq));
        printf("FAIL\n");
        return false;
    }

    // A = note index 9
    if (noteIdx != 9)
    {
        fprintf(stderr, "  note index: expected 9 (A), got %d\n", noteIdx);
        printf("FAIL\n");
        return false;
    }

    int octave = input.getTunerOctave();
    if (octave != 4)
    {
        fprintf(stderr, "  octave: expected 4, got %d\n", octave);
        printf("FAIL\n");
        return false;
    }

    input.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testTunerDetectsG4()
{
    printf("Test: tuner detects 392 Hz as G4... ");

    stellarr::InputBlock input;
    input.prepareToPlay(kSampleRate, kBlockSize);
    input.setTunerEnabled(true);

    for (int pass = 0; pass < 5; ++pass)
    {
        juce::AudioBuffer<float> buffer(2, kBlockSize);
        for (int i = 0; i < kBlockSize; ++i)
        {
            float sample = std::sin(kTwoPi * 392.0f * static_cast<float>(pass * kBlockSize + i) / static_cast<float>(kSampleRate));
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        juce::MidiBuffer midi;
        input.processBlock(buffer, midi);
    }

    // G = note index 7
    if (input.getTunerNoteIndex() != 7)
    {
        fprintf(stderr, "  expected note 7 (G), got %d\n", input.getTunerNoteIndex());
        printf("FAIL\n");
        return false;
    }

    if (input.getTunerOctave() != 4)
    {
        fprintf(stderr, "  expected octave 4, got %d\n", input.getTunerOctave());
        printf("FAIL\n");
        return false;
    }

    float cents = input.getTunerCents();
    if (std::abs(cents) > 10.0f)
    {
        fprintf(stderr, "  cents too far off: %f\n", static_cast<double>(cents));
        printf("FAIL\n");
        return false;
    }

    input.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testTunerNoise()
{
    printf("Test: tuner rejects noise... ");

    stellarr::InputBlock input;
    input.prepareToPlay(kSampleRate, kBlockSize);
    input.setTunerEnabled(true);

    juce::Random rng(42);
    for (int pass = 0; pass < 5; ++pass)
    {
        juce::AudioBuffer<float> buffer(2, kBlockSize);
        for (int i = 0; i < kBlockSize; ++i)
        {
            float sample = rng.nextFloat() * 2.0f - 1.0f;
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        juce::MidiBuffer midi;
        input.processBlock(buffer, midi);
    }

    // Should have low confidence or no detection
    if (input.getTunerConfidence() > 0.5f && input.getTunerNoteIndex() >= 0)
    {
        fprintf(stderr, "  noise should not produce high-confidence detection (confidence=%f, note=%d)\n",
            static_cast<double>(input.getTunerConfidence()), input.getTunerNoteIndex());
        printf("FAIL\n");
        return false;
    }

    input.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testTunerOutOfRange()
{
    printf("Test: tuner rejects out-of-range frequencies... ");

    stellarr::InputBlock input;
    input.prepareToPlay(kSampleRate, kBlockSize);
    input.setTunerEnabled(true);

    // 20 Hz — below guitar range
    for (int pass = 0; pass < 10; ++pass)
    {
        juce::AudioBuffer<float> buffer(2, kBlockSize);
        for (int i = 0; i < kBlockSize; ++i)
        {
            float sample = std::sin(kTwoPi * 20.0f * static_cast<float>(pass * kBlockSize + i) / static_cast<float>(kSampleRate));
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        juce::MidiBuffer midi;
        input.processBlock(buffer, midi);
    }

    if (input.getTunerNoteIndex() >= 0 && input.getTunerConfidence() > 0.3f)
    {
        fprintf(stderr, "  20 Hz should not be detected (note=%d, conf=%f)\n",
            input.getTunerNoteIndex(), static_cast<double>(input.getTunerConfidence()));
        printf("FAIL\n");
        return false;
    }

    input.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testTunerDisableClears()
{
    printf("Test: disabling tuner clears results... ");

    stellarr::InputBlock input;
    input.prepareToPlay(kSampleRate, kBlockSize);
    input.setTunerEnabled(true);

    // Feed 440 Hz
    for (int pass = 0; pass < 5; ++pass)
    {
        juce::AudioBuffer<float> buffer(2, kBlockSize);
        for (int i = 0; i < kBlockSize; ++i)
        {
            float sample = std::sin(kTwoPi * 440.0f * static_cast<float>(pass * kBlockSize + i) / static_cast<float>(kSampleRate));
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        juce::MidiBuffer midi;
        input.processBlock(buffer, midi);
    }

    // Should have detected something
    if (input.getTunerNoteIndex() < 0)
    {
        fprintf(stderr, "  tuner should detect before disable\n");
        printf("FAIL\n");
        return false;
    }

    // Disable
    input.setTunerEnabled(false);

    if (input.getTunerNoteIndex() != -1 ||
        input.getTunerFrequency() != 0.0f ||
        input.getTunerConfidence() != 0.0f)
    {
        fprintf(stderr, "  tuner state not cleared after disable\n");
        printf("FAIL\n");
        return false;
    }

    input.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testTunerBufferAccumulation()
{
    printf("Test: tuner accumulates samples before detection... ");

    stellarr::InputBlock input;
    input.prepareToPlay(kSampleRate, kBlockSize);
    input.setTunerEnabled(true);

    // Feed just one block (512 samples) — not enough for 2048 buffer
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    for (int i = 0; i < kBlockSize; ++i)
    {
        float sample = std::sin(kTwoPi * 440.0f * static_cast<float>(i) / static_cast<float>(kSampleRate));
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    juce::MidiBuffer midi;
    input.processBlock(buffer, midi);

    // Should not have detected yet (only 512 of 2048 samples)
    // Note: might still be -1 from initialization
    // After feeding enough samples it should detect
    for (int pass = 1; pass < 5; ++pass)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            float sample = std::sin(kTwoPi * 440.0f * static_cast<float>(pass * kBlockSize + i) / static_cast<float>(kSampleRate));
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        input.processBlock(buffer, midi);
    }

    // Now should have detected
    if (input.getTunerNoteIndex() < 0)
    {
        fprintf(stderr, "  should detect after enough samples\n");
        printf("FAIL\n");
        return false;
    }

    input.releaseResources();
    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    if (!testTunerDetects440())          ++failures;
    if (!testTunerDetectsG4())           ++failures;
    if (!testTunerNoise())               ++failures;
    if (!testTunerOutOfRange())          ++failures;
    if (!testTunerDisableClears())       ++failures;
    if (!testTunerBufferAccumulation())  ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
