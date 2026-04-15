#include "TestUtils.h"
#include "dsp/LoudnessMeter.h"

using stellarr::dsp::LoudnessMeter;

// Feed a sine wave into the meter for a given duration (in seconds).
static void feedSine(LoudnessMeter& meter, double sampleRate, int numChannels,
                     float frequency, float amplitude, double seconds)
{
    const int totalSamples = static_cast<int>(seconds * sampleRate);
    int remaining = totalSamples;
    while (remaining > 0)
    {
        const int chunk = juce::jmin(kBlockSize, remaining);
        juce::AudioBuffer<float> buffer(numChannels, chunk);
        buffer.clear();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < chunk; ++i)
            {
                float phase = static_cast<float>(totalSamples - remaining + i)
                              / static_cast<float>(sampleRate);
                buffer.setSample(ch, i,
                    amplitude * std::sin(static_cast<float>(juce::MathConstants<double>::twoPi)
                                         * frequency * phase));
            }
        }
        meter.process(buffer);
        remaining -= chunk;
    }
}

static bool testSilenceFloor()
{
    printf("Test: silent buffer reads silence floor... ");

    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    juce::AudioBuffer<float> buffer(2, static_cast<int>(0.5 * kSampleRate));
    buffer.clear();
    meter.process(buffer);

    const float m = meter.getMomentaryLufs();
    const float s = meter.getShortTermLufs();

    if (m > LoudnessMeter::kSilenceFloor + 1e-3f
        || s > LoudnessMeter::kSilenceFloor + 1e-3f)
    {
        fprintf(stderr, "  expected silence floor (-60), got momentary=%f short-term=%f\n",
                static_cast<double>(m), static_cast<double>(s));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testFullScaleSineStereo()
{
    printf("Test: full-scale 1 kHz stereo sine reads loud (near 0 LUFS)... ");

    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Feed a 1 kHz full-scale stereo sine for 1s (fills both windows).
    feedSine(meter, kSampleRate, 2, 1000.0f, 1.0f, 1.0);

    // Full-scale stereo 1 kHz should read within a few dB of 0 LUFS.
    // The exact value depends on the K-weighting curve at 1 kHz; accept
    // a generous range that catches gross calibration errors.
    const float lufs = meter.getShortTermLufs();
    if (lufs < -10.0f || lufs > 2.0f)
    {
        fprintf(stderr, "  expected loud reading, got %f\n", static_cast<double>(lufs));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testReducedAmplitudeMapsToLowerLufs()
{
    printf("Test: halving amplitude drops LUFS by ~6 dB... ");

    LoudnessMeter a;
    LoudnessMeter b;
    a.prepare(kSampleRate, 2);
    b.prepare(kSampleRate, 2);

    feedSine(a, kSampleRate, 2, 1000.0f, 1.0f, 1.0);
    feedSine(b, kSampleRate, 2, 1000.0f, 0.5f, 1.0);

    const float la = a.getShortTermLufs();
    const float lb = b.getShortTermLufs();
    const float diff = la - lb;

    // Halving amplitude = -6.02 dB (±1 dB tolerance).
    if (diff < 5.0f || diff > 7.0f)
    {
        fprintf(stderr, "  expected ~6 dB drop, got %f (la=%f lb=%f)\n",
                static_cast<double>(diff),
                static_cast<double>(la), static_cast<double>(lb));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testKWeightingAttenuatesLowFrequencies()
{
    printf("Test: K-weighting attenuates low frequencies vs 1 kHz... ");

    LoudnessMeter low;
    LoudnessMeter mid;
    low.prepare(kSampleRate, 2);
    mid.prepare(kSampleRate, 2);

    // 30 Hz is below the RLB high-pass corner (~38 Hz) and should be
    // attenuated relative to 1 kHz at the same amplitude.
    feedSine(low, kSampleRate, 2, 30.0f,   0.5f, 1.0);
    feedSine(mid, kSampleRate, 2, 1000.0f, 0.5f, 1.0);

    const float lLow = low.getShortTermLufs();
    const float lMid = mid.getShortTermLufs();

    if (lLow >= lMid - 3.0f)
    {
        fprintf(stderr, "  expected low freq to be >3 dB quieter, got low=%f mid=%f\n",
                static_cast<double>(lLow), static_cast<double>(lMid));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testStereoLouderThanMono()
{
    printf("Test: same signal in stereo reads ~3 dB louder than mono... ");

    LoudnessMeter mono;
    LoudnessMeter stereo;
    mono.prepare(kSampleRate, 2);
    stereo.prepare(kSampleRate, 2);

    // Mono: signal on ch 0, silence on ch 1.
    const int n = static_cast<int>(1.0 * kSampleRate);
    juce::AudioBuffer<float> monoBuf(2, n);
    monoBuf.clear();
    for (int i = 0; i < n; ++i)
        monoBuf.setSample(0, i, 0.5f * std::sin(static_cast<float>(juce::MathConstants<double>::twoPi)
                                                * 1000.0f * static_cast<float>(i) / static_cast<float>(kSampleRate)));
    mono.process(monoBuf);

    // Stereo: same signal on both channels.
    feedSine(stereo, kSampleRate, 2, 1000.0f, 0.5f, 1.0);

    const float lMono = mono.getShortTermLufs();
    const float lStereo = stereo.getShortTermLufs();
    const float diff = lStereo - lMono;

    // Summed energy across 2 channels = 2x single channel = +3.01 dB.
    if (diff < 2.0f || diff > 4.0f)
    {
        fprintf(stderr, "  expected +3 dB, got %f (mono=%f stereo=%f)\n",
                static_cast<double>(diff),
                static_cast<double>(lMono), static_cast<double>(lStereo));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testResetReturnsToFloor()
{
    printf("Test: reset() clears meter back to silence floor... ");

    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    feedSine(meter, kSampleRate, 2, 1000.0f, 1.0f, 1.0);
    if (meter.getShortTermLufs() < -30.0f)
    {
        fprintf(stderr, "  precondition failed: meter should have a reading before reset\n");
        printf("FAIL\n");
        return false;
    }

    meter.reset();

    const float m = meter.getMomentaryLufs();
    const float s = meter.getShortTermLufs();
    if (m > LoudnessMeter::kSilenceFloor + 1e-3f
        || s > LoudnessMeter::kSilenceFloor + 1e-3f)
    {
        fprintf(stderr, "  expected silence floor after reset, got momentary=%f short-term=%f\n",
                static_cast<double>(m), static_cast<double>(s));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testMomentaryRespondsFasterThanShortTerm()
{
    printf("Test: momentary window reacts faster than short-term... ");

    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    // Feed exactly 0.4 s (momentary fully populated, short-term only ~13%).
    feedSine(meter, kSampleRate, 2, 1000.0f, 1.0f, 0.4);

    const float m = meter.getMomentaryLufs();
    const float s = meter.getShortTermLufs();

    // Momentary should be at the steady-state reading while short-term is
    // still filling with zeros, so momentary should be louder.
    if (m <= s + 3.0f)
    {
        fprintf(stderr, "  expected momentary > short-term+3, got momentary=%f short-term=%f\n",
                static_cast<double>(m), static_cast<double>(s));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testConsistencyAcrossSampleRates()
{
    printf("Test: same signal reads similar LUFS at 44.1k / 48k / 96k... ");

    const double rates[] = { 44100.0, 48000.0, 96000.0 };
    float readings[3];

    for (int i = 0; i < 3; ++i)
    {
        LoudnessMeter meter;
        meter.prepare(rates[i], 2);
        feedSine(meter, rates[i], 2, 1000.0f, 0.5f, 1.0);
        readings[i] = meter.getShortTermLufs();
    }

    for (int i = 1; i < 3; ++i)
    {
        if (std::abs(readings[i] - readings[0]) > 1.0f)
        {
            fprintf(stderr, "  reading at rate %f (%f) diverges from 44.1k (%f)\n",
                    rates[i],
                    static_cast<double>(readings[i]),
                    static_cast<double>(readings[0]));
            printf("FAIL\n");
            return false;
        }
    }

    printf("PASS\n");
    return true;
}

static bool testEmptyBufferNoOp()
{
    printf("Test: empty buffer doesn't crash and leaves floor... ");

    LoudnessMeter meter;
    meter.prepare(kSampleRate, 2);

    juce::AudioBuffer<float> empty(2, 0);
    meter.process(empty);

    if (meter.getMomentaryLufs() > LoudnessMeter::kSilenceFloor + 1e-3f)
    {
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    if (!testSilenceFloor())                      ++failures;
    if (!testFullScaleSineStereo())               ++failures;
    if (!testReducedAmplitudeMapsToLowerLufs())   ++failures;
    if (!testKWeightingAttenuatesLowFrequencies()) ++failures;
    if (!testStereoLouderThanMono())              ++failures;
    if (!testResetReturnsToFloor())               ++failures;
    if (!testMomentaryRespondsFasterThanShortTerm()) ++failures;
    if (!testConsistencyAcrossSampleRates())      ++failures;
    if (!testEmptyBufferNoOp())                   ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
