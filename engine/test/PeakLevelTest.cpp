#include "TestUtils.h"

static bool testPeakWithKnownSignal()
{
    printf("Test: peak level correct for known sine amplitude... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer); // amplitude 1.0

    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    float peak = proc.getOutputPeakLevel();

    // Sine peak should be close to 1.0
    if (peak < 0.9f || peak > 1.01f)
    {
        fprintf(stderr, "  expected peak ~1.0, got %f\n", static_cast<double>(peak));
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testPeakSilentBuffer()
{
    printf("Test: silent buffer produces peak of 0... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    float peak = proc.getOutputPeakLevel();

    if (peak > 1e-6f)
    {
        fprintf(stderr, "  expected peak 0, got %f\n", static_cast<double>(peak));
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testPeakEmptyBuffer()
{
    printf("Test: empty buffer (0 samples) produces peak of 0... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, 0);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    float peak = proc.getOutputPeakLevel();

    if (peak > 1e-6f)
    {
        fprintf(stderr, "  expected peak 0, got %f\n", static_cast<double>(peak));
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testPeakClipping()
{
    printf("Test: clipping detected when samples exceed 1.0... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    buffer.applyGain(2.0f); // amplitude 2.0 = clipping

    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    float peak = proc.getOutputPeakLevel();

    if (peak <= 1.0f)
    {
        fprintf(stderr, "  expected peak > 1.0 (clipping), got %f\n", static_cast<double>(peak));
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    if (!testPeakWithKnownSignal()) ++failures;
    if (!testPeakSilentBuffer())    ++failures;
    if (!testPeakEmptyBuffer())     ++failures;
    if (!testPeakClipping())        ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
