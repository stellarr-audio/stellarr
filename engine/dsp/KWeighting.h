#pragma once

#include <juce_dsp/juce_dsp.h>

namespace stellarr::dsp
{

/**
 * K-weighting filter for ITU-R BS.1770 LUFS measurement.
 * Two biquads in series:
 *   1. Pre-filter: high-shelf at ~1500 Hz, +4 dB
 *   2. RLB filter: high-pass at ~38 Hz, -6 dB/oct
 * Coefficients are sample-rate dependent; call prepare() with the rate.
 */
class KWeighting
{
public:
    void prepare(double sampleRate)
    {
        // Pre-filter (high-shelf) coefficients per BS.1770 at 48 kHz, scaled here.
        // Using bilinear-transform derived coeffs from ITU-R BS.1770-4 Annex 1.
        const double f0 = 1681.974450955533;
        const double G  = 3.999843853973347;
        const double Q  = 0.7071752369554196;

        const double K  = std::tan(juce::MathConstants<double>::pi * f0 / sampleRate);
        const double Vh = std::pow(10.0, G / 20.0);
        const double Vb = std::pow(Vh, 0.4996667741545416);

        const double a0 = 1.0 + K / Q + K * K;
        preB0 = (Vh + Vb * K / Q + K * K) / a0;
        preB1 = 2.0 * (K * K - Vh) / a0;
        preB2 = (Vh - Vb * K / Q + K * K) / a0;
        preA1 = 2.0 * (K * K - 1.0) / a0;
        preA2 = (1.0 - K / Q + K * K) / a0;

        // RLB filter (high-pass) coefficients per BS.1770
        const double f0r = 38.13547087602444;
        const double Qr  = 0.5003270373238773;
        const double Kr  = std::tan(juce::MathConstants<double>::pi * f0r / sampleRate);
        const double a0r = 1.0 + Kr / Qr + Kr * Kr;

        rlbB0 = 1.0;
        rlbB1 = -2.0;
        rlbB2 = 1.0;
        rlbA1 = 2.0 * (Kr * Kr - 1.0) / a0r;
        rlbA2 = (1.0 - Kr / Qr + Kr * Kr) / a0r;

        reset();
    }

    void reset()
    {
        preX1 = preX2 = preY1 = preY2 = 0.0f;
        rlbX1 = rlbX2 = rlbY1 = rlbY2 = 0.0f;
    }

    float processSample(float x)
    {
        // Pre-filter
        float y = static_cast<float>(preB0) * x
                + static_cast<float>(preB1) * preX1
                + static_cast<float>(preB2) * preX2
                - static_cast<float>(preA1) * preY1
                - static_cast<float>(preA2) * preY2;
        preX2 = preX1; preX1 = x;
        preY2 = preY1; preY1 = y;

        // RLB filter
        float z = static_cast<float>(rlbB0) * y
                + static_cast<float>(rlbB1) * rlbX1
                + static_cast<float>(rlbB2) * rlbX2
                - static_cast<float>(rlbA1) * rlbY1
                - static_cast<float>(rlbA2) * rlbY2;
        rlbX2 = rlbX1; rlbX1 = y;
        rlbY2 = rlbY1; rlbY1 = z;
        return z;
    }

private:
    double preB0 = 0, preB1 = 0, preB2 = 0, preA1 = 0, preA2 = 0;
    double rlbB0 = 0, rlbB1 = 0, rlbB2 = 0, rlbA1 = 0, rlbA2 = 0;
    float preX1 = 0, preX2 = 0, preY1 = 0, preY2 = 0;
    float rlbX1 = 0, rlbX2 = 0, rlbY1 = 0, rlbY2 = 0;
};

} // namespace stellarr::dsp
