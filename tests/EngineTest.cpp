// Lokaler Test der DSP-Engine (ohne GUI): cmake -S . -B build -DKNACK_TEST_SRC=tests/EngineTest.cpp
#include "../Source/Engine.h"
#include <cstdio>

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf("FAIL: %s\n", msg); ++fails; } else std::printf("ok:   %s\n", msg); } while (0)

static float peakOf (const juce::AudioBuffer<float>& b, int from = 0)
{
    float p = 0; for (int c = 0; c < b.getNumChannels(); ++c) p = juce::jmax (p, b.getMagnitude (c, from, b.getNumSamples() - from)); return p;
}

static void sine (juce::AudioBuffer<float>& b, double sr, float amp, double f = 220.0, int offset = 0)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (c, i, amp * (float) std::sin (2.0 * 3.14159265358979 * f * (double) (i + offset) / sr));
}

int main()
{
    const double sr = 48000.0;
    const int block = 512;
    KnackEngine e;
    e.prepare (sr, block, 2);
    std::printf("Latenz bei HQ: %d Samples\n", e.getLatencySamples (true));

    juce::AudioBuffer<float> buf (2, block);
    auto run = [&] (KnackParams p, float amp, int blocks = 40)
    {
        for (int b = 0; b < blocks; ++b) { sine (buf, sr, amp, 220.0, b * block); e.process (buf, p); }
        return peakOf (buf);
    };

    // 1) Ceiling: nie lauter als -1 dBFS, auch bei maximalem Clip + Input
    { KnackParams p; p.clip = 1.0f; p.inputDb = 24.0f; p.heat = 1.0f;
      const float pk = run (p, 1.0f);
      CHECK (pk <= juce::Decibels::decibelsToGain (-1.0f) + 1e-4f, "Peak <= -1 dBFS bei Maximum"); }

    // 2) OUT zurueckziehen: Ceiling sinkt (-6 -> -7 dBFS) und Limiter zeigt GR
    { KnackParams p; p.clip = 1.0f; p.outputDb = -6.0f;
      const float pk = run (p, 1.0f);
      CHECK (pk <= juce::Decibels::decibelsToGain (-7.0f) + 1e-4f, "OUT -6 => Peak <= -7 dBFS");
      CHECK (e.limGrDb.load() > 0.5f, "Limiter GR sichtbar"); }

    // 3) CLIP=0, Mix=1, Heat=0: ziemlich linear (kleines Signal bleibt fast gleich)
    { KnackParams p; p.clip = 0.0f; p.shape = 0.0f; p.hq = false;
      const float pk = run (p, 0.05f);
      CHECK (std::abs (pk - 0.05f) < 0.003f, "CLIP 0, kleines Signal ~ unveraendert"); }

    // 4) Mix=0 ist exakt Dry, um die Oversampling-Latenz verzoegert (phasenrichtig)
    { KnackParams p; p.clip = 1.0f; p.mix = 0.0f; p.hq = true;
      e.reset();
      const int L = e.getLatencySamples (true);
      for (int b = 0; b < 10; ++b) { sine (buf, sr, 0.3f, 220.0, b * block); e.process (buf, p); }
      sine (buf, sr, 0.3f, 220.0, 10 * block); e.process (buf, p);
      float maxDiff = 0;
      for (int i = L; i < block; ++i)
          maxDiff = juce::jmax (maxDiff, std::abs (buf.getSample (0, i) - 0.3f * (float) std::sin (2.0 * 3.14159265358979 * 220.0 * (double) (10 * block + i - L) / sr)));
      std::printf("   (Mix 0, Latenz %d, maxDiff %.5f)\n", L, maxDiff);
      CHECK (maxDiff < 0.002f, "Mix 0 = Dry, latenzrichtig"); }

    // 5) Hard-Clip bei viel Drive: Ausgang nahe Rechteck (hoher Anteil Samples nahe Ceiling)
    { KnackParams p; p.clip = 1.0f; p.shape = 1.0f; p.hq = false; p.outputDb = 0.0f;
      run (p, 0.5f, 20);
      sine (buf, sr, 0.5f, 220.0, 20 * block); e.process (buf, p);
      const float ceil = juce::Decibels::decibelsToGain (-1.0f);
      int near = 0; for (int i = 0; i < block; ++i) if (std::abs (buf.getSample (0, i)) > 0.9f * ceil) ++near;
      CHECK (near > block * 0.6f, "CLIP 100 % + HARD: Welle fast ein Rechteck"); }

    // 6) Soft vs Hard: bei gleichem Drive hat Soft mehr Kruemmung -> geringerer RMS
    { KnackParams ps; ps.clip = 0.5f; ps.shape = 0.0f; ps.hq = false;
      KnackParams ph = ps; ph.shape = 1.0f;
      e.reset(); run (ps, 0.5f, 20); sine (buf, sr, 0.5f, 220.0, 20 * block); e.process (buf, ps);
      const float rmsS = buf.getRMSLevel (0, 0, block);
      e.reset(); run (ph, 0.5f, 20); sine (buf, sr, 0.5f, 220.0, 20 * block); e.process (buf, ph);
      const float rmsH = buf.getRMSLevel (0, 0, block);
      std::printf("   RMS soft %.3f / hard %.3f\n", rmsS, rmsH);
      CHECK (rmsH > rmsS, "Hard klingt dichter (mehr RMS) als Soft"); }

    // 7) Heat erzeugt kein DC am Ausgang (187.5 Hz = ganze Perioden pro Block)
    { KnackParams p; p.clip = 0.3f; p.heat = 1.0f; p.hq = false;
      for (int b = 0; b < 200; ++b) { sine (buf, sr, 0.4f, 187.5, b * block); e.process (buf, p); }
      double sum = 0; for (int i = 0; i < block; ++i) sum += buf.getSample (0, i);
      CHECK (std::abs (sum / block) < 0.01, "Heat: kein DC-Offset"); }

    // 8) Calibrate: Eingang -20 dBFS Peak -> Input +14 dB (Ziel -6 dBFS)
    { e.reset(); KnackParams p; e.startCalibration();
      float res = 0; bool got = false;
      for (int b = 0; b < 600 && ! got; ++b) { sine (buf, sr, 0.1f, 220.0, b * block); e.process (buf, p); got = e.popCalibrationResult (res); }
      std::printf("   Calibrate Ergebnis: %.2f dB\n", res);
      CHECK (got && std::abs (res - 14.0f) < 0.3f, "Calibrate: -20 dBFS -> +14 dB"); }

    // 9) Calibrate ohne Signal bricht ab
    { e.reset(); KnackParams p; e.startCalibration(); float res = 0; bool got = false;
      juce::AudioBuffer<float> silent (2, block); silent.clear();
      for (int b = 0; b < 1400; ++b) { silent.clear(); e.process (silent, p); got |= e.popCalibrationResult (res); }
      CHECK (! got && ! e.isCalibrating(), "Calibrate ohne Signal: kein Ergebnis, Abbruch"); }

    // 10) Mono
    { KnackEngine m; m.prepare (sr, block, 1); juce::AudioBuffer<float> mb (1, block); KnackParams p;
      for (int b = 0; b < 20; ++b) { sine (mb, sr, 0.5f, 220.0, b * block); m.process (mb, p); }
      CHECK (peakOf (mb) > 0.1f, "Mono laeuft"); }

    std::printf(fails ? "\n%d Test(s) FEHLGESCHLAGEN\n" : "\nAlle Tests bestanden\n", fails);
    return fails;
}
