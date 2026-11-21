#include "Engine.h"

using namespace juce;

void KnackEngine::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    sr = sampleRate;
    maxBlock = jmax (16, maxBlockSize);
    numCh = jlimit (1, 2, numChannels);

    os = std::make_unique<dsp::Oversampling<float>> ((size_t) numCh, (size_t) 2,
            dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) maxBlock);
    osLatency = (int) std::lround (os->getLatencyInSamples());

    clipArr.assign ((size_t) maxBlock, 0.0f);
    shapeArr.assign ((size_t) maxBlock, 0.0f);
    dryRing.setSize (2, ringMask + 1);
    dryBuf.setSize (2, maxBlock);

    inS.reset (sr, 0.03);
    clipS.reset (sr, 0.03);
    shapeS.reset (sr, 0.03);
    mixS.reset (sr, 0.03);
    ceilS.reset (sr, 0.03);
    reset();
}

void KnackEngine::reset()
{
    if (os) os->reset();
    dryRing.clear();
    ringWrite = 0;
    for (int c = 0; c < 2; ++c) dcX[c] = dcY[c] = 0.0f;
    limGain = 1.0f;
    calActive = false;
    inS.setCurrentAndTargetValue (inS.getTargetValue());
    clipS.setCurrentAndTargetValue (clipS.getTargetValue());
    shapeS.setCurrentAndTargetValue (shapeS.getTargetValue());
    mixS.setCurrentAndTargetValue (mixS.getTargetValue());
    ceilS.setCurrentAndTargetValue (ceilS.getTargetValue());
}

void KnackEngine::process (AudioBuffer<float>& buffer, const KnackParams& p)
{
    const int total = buffer.getNumSamples();
    const int chans = jmin (numCh, buffer.getNumChannels());
    if (chans <= 0 || total <= 0)
        return;

    if (p.hq != lastHq)
    {
        if (p.hq) os->reset();
        lastHq = p.hq;
    }

    for (int start = 0; start < total; start += maxBlock)
    {
        const int len = jmin (maxBlock, total - start);
        AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), chans, start, len);
        processChunk (sub, p);
    }
}

void KnackEngine::runCalibration (const AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    float peak = 0.0f;
    for (int c = 0; c < buffer.getNumChannels(); ++c)
        peak = jmax (peak, buffer.getMagnitude (c, 0, n));

    calTotalSamples += n;
    if (peak > 0.001f)   // nur echtes Signal zaehlt (> -60 dBFS)
    {
        calPeak = jmax (calPeak, peak);
        calActiveSamples += n;
    }

    const int needActive = (int) (3.0 * sr);
    const int maxTotal   = (int) (12.0 * sr);
    calProgress = jlimit (0.0f, 1.0f, (float) calActiveSamples / (float) needActive);

    if (calActiveSamples >= needActive)
    {
        const float db = KnackDSP::calibTarget - Decibels::gainToDecibels (calPeak);
        calResultDb = jlimit (-24.0f, 24.0f, db);
        calDone = true;
        calActive = false;
    }
    else if (calTotalSamples >= maxTotal)
    {
        // Kein (genug) Signal: abbrechen, Input bleibt wie er ist
        calActive = false;
        calProgress = 0.0f;
    }
}

void KnackEngine::processChunk (AudioBuffer<float>& buffer, const KnackParams& p)
{
    const int n = buffer.getNumSamples();
    const int chans = buffer.getNumChannels();

    // ---- Calibrate (misst das unbearbeitete Eingangssignal) ----
    if (calCancel.exchange (false)) { calActive = false; calProgress = 0.0f; }
    if (calRequest.exchange (false))
    {
        calActive = true;
        calPeak = 0.0f;
        calActiveSamples = calTotalSamples = 0;
        calProgress = 0.0f;
    }
    if (calActive.load())
        runCalibration (buffer);

    // ---- Parameter-Rampen ----
    inS.setTargetValue (Decibels::decibelsToGain (p.inputDb));
    clipS.setTargetValue (p.clip);
    shapeS.setTargetValue (p.shape);
    mixS.setTargetValue (p.mix);
    ceilS.setTargetValue (Decibels::decibelsToGain (KnackDSP::ceilingDb + jlimit (-24.0f, 0.0f, p.outputDb)));

    // ---- Input-Gain + Anzeige ----
    float inPk = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float g = inS.getNextValue();
        clipArr[(size_t) i] = clipS.getNextValue();
        shapeArr[(size_t) i] = shapeS.getNextValue();
        for (int c = 0; c < chans; ++c)
        {
            float* d = buffer.getWritePointer (c);
            d[i] *= g;
            inPk = jmax (inPk, std::abs (d[i]));
        }
    }
    {
        const float prev = inPeak.load();
        inPeak = inPk > prev ? inPk : prev * 0.9f;
    }

    // ---- Dry-Kopie (verzoegert um die Oversampling-Latenz, damit Mix phasenrichtig bleibt) ----
    const int dryDelay = p.hq ? osLatency : 0;
    auto& dry = dryBuf;
    {
        int w = ringWrite;
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < chans; ++c)
            {
                dryRing.setSample (c, w & ringMask, buffer.getSample (c, i));
                dry.setSample (c, i, dryRing.getSample (c, (w - dryDelay) & ringMask));
            }
            ++w;
        }
        ringWrite = w & ringMask;
    }

    // ---- Wet: Drive -> Clip (4x Oversampling im HQ-Modus) ----
    float maxRatio = 1.0f;
    auto processWet = [&] (dsp::AudioBlock<float> block, int factor)
    {
        const int ns = (int) block.getNumSamples();
        for (int j = 0; j < ns; ++j)
        {
            const int idx = jmin (n - 1, j / factor);
            const float clip = clipArr[(size_t) idx];
            const float drive = KnackDSP::driveGain (clip);
            const float k = KnackDSP::curveK (shapeArr[(size_t) idx]);
            for (int c = 0; c < chans; ++c)
            {
                float* d = block.getChannelPointer ((size_t) c);
                const float xd = d[j] * drive;
                const float y = KnackDSP::shape (xd, k);
                const float ax = std::abs (xd), ay = std::abs (y);
                if (ay > 1e-6f && ax > ay)
                    maxRatio = jmax (maxRatio, ax / ay);
                d[j] = y;
            }
        }
    };

    dsp::AudioBlock<float> block (buffer);
    if (p.hq)
    {
        auto up = os->processSamplesUp (block);
        processWet (up, 4);
        os->processSamplesDown (block);
    }
    else
    {
        processWet (block, 1);
    }

    {
        const float red = Decibels::gainToDecibels (maxRatio);
        const float prev = clipGrDb.load();
        clipGrDb = red > prev ? red : prev * 0.88f;
    }

    // ---- DC-Blocker, Mix, Limiter ----
    const float dcR = 1.0f - 20.0f / (float) sr;
    float minGain = 1.0f, outPk = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float m = mixS.getNextValue();
        const float ceiling = ceilS.getNextValue();
        float peak = 0.0f;
        float v[2] {};
        for (int c = 0; c < chans; ++c)
        {
            const float x = buffer.getSample (c, i);
            const float y = x - dcX[c] + dcR * dcY[c];
            dcX[c] = x;
            dcY[c] = y;
            v[c] = dry.getSample (c, i) * (1.0f - m) + y * m;
            peak = jmax (peak, std::abs (v[c]));
        }

        // Peak-Limiter: sofortiger Attack, 80 ms Release, danach harte Grenze (wie in den anderen Plugins)
        const float rel = std::exp (-1.0f / (0.08f * (float) sr));
        const float target = peak * limGain > ceiling ? ceiling / jmax (1e-9f, peak) : 1.0f;
        limGain = target < limGain ? target : target + (limGain - target) * rel;
        minGain = jmin (minGain, limGain);

        for (int c = 0; c < chans; ++c)
        {
            const float o = jlimit (-ceiling, ceiling, v[c] * limGain);
            buffer.setSample (c, i, o);
            outPk = jmax (outPk, std::abs (o));
        }
    }

    {
        const float red = -Decibels::gainToDecibels (minGain);
        const float prev = limGrDb.load();
        limGrDb = red > prev ? red : prev * 0.88f;
        const float po = outPeak.load();
        outPeak = outPk > po ? outPk : po * 0.9f;
    }
}
