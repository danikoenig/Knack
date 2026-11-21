#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <vector>

namespace KnackDSP
{
    constexpr float maxDriveDb   = 24.0f;    // CLIP 100 % = +24 dB in den Clipper
    constexpr float ceilingDb    = -1.0f;    // OUT-Regler 0 = -1 dBFS
    constexpr float calibTarget  = -6.0f;    // Calibrate: lauteste Spitze landet bei -6 dBFS

    // SHAPE: eine Kurve von soft (k klein) bis hart (k gross): y = x / (1 + |x|^k)^(1/k)
    inline float curveK (float shapeAmt) { return 1.6f + 10.4f * shapeAmt; }   // 0 = soft, 1 = hard
    inline float driveGain (float clip) { return juce::Decibels::decibelsToGain (clip * maxDriveDb); }

    inline float shape (float x, float k)
    {
        const float ax = std::abs (x);
        return x / std::pow (1.0f + std::pow (ax, k), 1.0f / k);
    }

    // Heat: asymmetrische Roehren-/Tape-Saettigung, kleines Signal bleibt auf Pegel
    inline float heatShape (float x, float heat)
    {
        constexpr float a = 2.2f, b = 0.22f;
        static const float t0 = std::tanh (b);
        static const float norm = 1.0f / (a * (1.0f - t0 * t0));
        const float s = (std::tanh (a * x + b) - t0) * norm * a;   // Steigung 1 bei 0
        return x + heat * (s - x);
    }
}

struct KnackParams
{
    float inputDb  = 0.0f;
    float clip     = 0.35f;   // 0..1  Drive in den Clipper
    float shape    = 0.5f;    // 0..1  0 = Soft, 1 = Hard
    float heat     = 0.0f;    // 0..1
    float mix      = 1.0f;    // 0..1
    float outputDb = 0.0f;    // -12..0 (0 = -1 dBFS)
    bool  hq       = true;    // 4x Oversampling
};

class KnackEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    int  getLatencySamples (bool hq) const { return hq ? osLatency : 0; }
    void process (juce::AudioBuffer<float>& buffer, const KnackParams& p);

    // Calibrate (Auto-Input)
    void startCalibration()  { calRequest = true; }
    void cancelCalibration() { calCancel = true; }
    bool isCalibrating() const { return calActive.load(); }
    float getCalProgress() const { return calProgress.load(); }
    bool popCalibrationResult (float& inputDb)
    {
        if (! calDone.exchange (false)) return false;
        inputDb = calResultDb.load();
        return true;
    }

    // Anzeigen (linear bzw. dB)
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f }, clipGrDb { 0.0f }, limGrDb { 0.0f };

private:
    void processChunk (juce::AudioBuffer<float>& buffer, const KnackParams& p);
    void runCalibration (const juce::AudioBuffer<float>& buffer);

    double sr = 44100.0;
    int maxBlock = 512, numCh = 2, osLatency = 0;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;

    juce::SmoothedValue<float> inS, clipS, shapeS, heatS, mixS, ceilS;
    std::vector<float> clipArr, heatArr, shapeArr;
    juce::AudioBuffer<float> dryRing, dryBuf;
    int ringWrite = 0, ringMask = 1023;
    bool lastHq = true;
    float dcX[2] {}, dcY[2] {};
    float limGain = 1.0f;

    // Calibrate
    std::atomic<bool> calRequest { false }, calCancel { false }, calActive { false }, calDone { false };
    std::atomic<float> calProgress { 0.0f }, calResultDb { 0.0f };
    float calPeak = 0.0f;
    int calActiveSamples = 0, calTotalSamples = 0;
};
