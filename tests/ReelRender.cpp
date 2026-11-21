// Reel-Renderer fuer Knack: spielt einen festen Ablauf im echten Plugin ab (Calibrate, CLIP, Soft/Hard, Mix),
// schickt einen synthetischen Drum-Loop durch die echte Engine (WAV) und rendert die GUI-Frames (JPG, 30 fps).
// Schreibt events.json (Layout + Cursor) fuer tools/reel/compose.py.
// Aufruf: ReelRender <out-dir>
#include "../Source/PluginProcessor.h"
#define private public   // nur Test: Zugriff auf Editor-Komponenten
#include "../Source/PluginEditor.h"
#undef private
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>
#include <functional>

static constexpr double FPS = 30.0, SR = 48000.0, BPM = 124.0, BEAT = 60.0 / BPM;
static constexpr double TOTAL = 30.5, GUI_UNTIL = 27.0;

struct ReelAccess
{
    static void tick (KnackEditor& e) { e.timerCallback(); }
};

static void setParam (KnackProcessor& p, const char* id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}
static double ease (double x) { x = juce::jlimit (0.0, 1.0, x); return x * x * (3.0 - 2.0 * x); }
static double ramp (double t, double t0, double t1, double a, double b) { return a + (b - a) * ease ((t - t0) / (t1 - t0)); }

//==============================================================================
// Synthetischer Beat (124 BPM): Kick, Clap, Hats, Bass. Absichtlich leise, wie ein Mix ohne Pegel.
static juce::AudioBuffer<float> makeBeat (int total)
{
    juce::AudioBuffer<float> b (2, total);
    b.clear();
    juce::Random rnd (42);
    auto add = [&] (int pos, float l, float r) { if (pos >= 0 && pos < total) { b.addSample (0, pos, l); b.addSample (1, pos, r); } };
    const int beatS = (int) std::round (BEAT * SR);
    const int beats = total / beatS + 1;

    for (int k = 0; k < beats; ++k)
    {
        const int t0 = k * beatS;
        // Kick (jede Viertel)
        { double ph = 0;
          for (int i = 0; i < (int) (0.38 * SR); ++i)
          { const double t = i / SR; const double f = 46.0 + 110.0 * std::exp (-t / 0.028);
            ph += 2.0 * juce::MathConstants<double>::pi * f / SR;
            const float s = (float) (std::sin (ph) * std::exp (-t / 0.16) * 0.95 + (t < 0.003 ? 0.35 * rnd.nextFloat() : 0.0));
            add (t0 + i, s, s); } }
        // Clap (2 und 4)
        if (k % 2 == 1)
        { double lp = 0, hp = 0, prev = 0;
          for (int i = 0; i < (int) (0.22 * SR); ++i)
          { const double t = i / SR;
            double env = std::exp (-t / 0.07) * 0.55;
            for (double off : { 0.0, 0.011, 0.022 }) if (t >= off && t < off + 0.008) env += 0.7 * (1.0 - (t - off) / 0.008);
            const double n = rnd.nextFloat () * 2.0 - 1.0;
            lp += 0.35 * (n - lp); hp = lp - prev * 0.0; prev = lp;
            const float s = (float) (lp * env * 0.55);
            add (t0 + i, s * 0.9f, s); } }
        // Hats: 16tel leise, Off-Beat offen
        for (int q = 0; q < 4; ++q)
        { const int tp = t0 + q * beatS / 4;
          const bool openHat = (q == 2);
          const double vel = openHat ? 0.34 : (q % 2 == 1 ? 0.16 : 0.1);
          double prevn = 0;
          for (int i = 0; i < (int) ((openHat ? 0.16 : 0.045) * SR); ++i)
          { const double t = i / SR; const double n = rnd.nextFloat () * 2.0 - 1.0; const double hpn = n - prevn; prevn = n;
            const float s = (float) (hpn * std::exp (-t / (openHat ? 0.07 : 0.012)) * vel);
            add (tp + i, s * 0.85f, s * 1.0f); } }
        // Bass: Off-Beat-8tel, A Moll Gruppe (A1, A1, C2, G1), Sidechain-artig kurz nach der Kick
        { static const double notes[4] = { 55.0, 55.0, 65.41, 49.0 };
          const double f = notes[k % 4];
          const int tp = t0 + beatS / 2;
          double ph = 0, lp = 0;
          for (int i = 0; i < (int) (0.40 * SR); ++i)
          { const double t = i / SR; ph += f / SR; ph -= std::floor (ph);
            const double saw = 2.0 * ph - 1.0;
            lp += (0.045 + 0.10 * std::exp (-t / 0.1)) * (saw - lp);
            const float s = (float) (lp * std::min (1.0, t / 0.004) * std::exp (-t / 0.22) * 0.55);
            add (tp + i, s, s); } }
    }
    // ca. -14 dBFS Spitze, wie ein Mix ohne Pegel
    const float pk = b.getMagnitude (0, total);
    b.applyGain (0.2f / pk);
    return b;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) { std::printf ("usage: ReelRender <out-dir>\n"); return 1; }
    const juce::File outDir { juce::String (argv[1]) };
    outDir.getChildFile ("frames").createDirectory();

    const int totalSamples = (int) std::round (TOTAL * SR);
    auto beat = makeBeat (totalSamples);

    KnackProcessor p;
    p.prepareToPlay (SR, 512);
    setParam (p, ParamID::input, -3.0f);
    setParam (p, ParamID::clip, 0.0f);
    setParam (p, ParamID::shape, 0.5f);
    setParam (p, ParamID::mix, 1.0f);
    setParam (p, ParamID::output, 0.0f);
    setParam (p, ParamID::hq, 1.0f);

    std::unique_ptr<KnackEditor> edp (new KnackEditor (p));
    auto& ed = *edp;
    ed.setSize (600, 1504);   // 2x Designgroesse (300 x 752), scharfe Frames
    ReelAccess::tick (ed);

    // Layout fuer compose.py
    auto* lay = new juce::DynamicObject();
    auto put = [lay] (const char* k, juce::Rectangle<int> r)
    { juce::Array<juce::var> a { r.getX(), r.getY(), r.getWidth(), r.getHeight() }; lay->setProperty (k, a); };
    put ("gui", ed.getLocalBounds());
    put ("scope", ed.scope.getBounds());
    put ("shape", ed.shapeSlider.getBounds());
    put ("knob", ed.clipKnob.getBounds());
    put ("mix", ed.mixSlider.getBounds());
    put ("input", ed.inputKnob.getBounds());
    put ("inmeter", ed.inMeter.getBounds());
    put ("calibrate", ed.calibrateBtn.getBounds());
    put ("out", ed.outSlider.getBounds().getUnion (ed.limMeter.getBounds()).withRight (ed.getWidth() - 10));
    put ("hq", ed.hqBtn.getBounds());

    // Cursor-Ziele (Editor-Koordinaten), pro Frame
    const auto knobC = ed.clipKnob.getBounds().getCentre().toFloat().translated (0.0f, -18.0f);
    const auto calC = ed.calibrateBtn.getBounds().getCentre().toFloat();
    auto thumbX = [&] (juce::Slider& s, double v01)
    {
        auto b = s.getBounds().toFloat().withTrimmedRight (54.0f * 2.0f);
        return b.getX() + b.getWidth() * (float) v01;
    };
    juce::Array<juce::var> cursor;

    juce::AudioBuffer<float> out (2, totalSamples);
    out.clear();
    int pos = 0;
    bool calStarted = false;
    const int numFrames = (int) std::round (TOTAL * FPS);

    auto autom = [&] (double t)
    {
        double clip = 0.0;
        if (t < 6.2) clip = 0.0;
        else if (t < 10.8) clip = ramp (t, 6.4, 10.8, 0.0, 0.40);
        else if (t < 16.0) clip = ramp (t, 11.2, 16.0, 0.40, 1.0);
        else if (t < 17.0) clip = ramp (t, 16.0, 17.0, 1.0, 0.70);
        else if (t < 21.5) clip = 0.70;
        else clip = ramp (t, 21.5, 22.5, 0.70, 0.85);
        double shape = 0.5;
        if (t >= 17.2 && t < 18.6) shape = ramp (t, 17.2, 18.6, 0.5, 0.0);
        else if (t >= 18.6 && t < 20.2) shape = ramp (t, 18.6, 20.2, 0.0, 1.0);
        else if (t >= 20.2) shape = ramp (t, 20.2, 21.0, 1.0, 0.5);
        double mix = 1.0;
        if (t >= 23.0) mix = ramp (t, 23.0, 25.8, 1.0, 0.4);
        setParam (p, ParamID::clip, (float) clip);
        setParam (p, ParamID::shape, (float) shape);
        setParam (p, ParamID::mix, (float) mix);
        return std::array<double, 3> { clip, shape, mix };
    };

    for (int f = 0; f < numFrames; ++f)
    {
        const double t = f / FPS;
        const auto v = autom (t);
        if (! calStarted && t >= 2.7) { p.getEngine().startCalibration(); calStarted = true; }

        const int endSample = juce::jmin (totalSamples, (int) std::round ((f + 1) / FPS * SR));
        while (pos < endSample)
        {
            const int n = juce::jmin (512, endSample - pos);
            juce::AudioBuffer<float> blk (2, n);
            for (int c = 0; c < 2; ++c) blk.copyFrom (c, 0, beat, c, pos, n);
            juce::MidiBuffer midi;
            p.processBlock (blk, midi);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, blk, c, 0, n);
            pos += n;
        }
        juce::MessageManager::getInstance()->runDispatchLoopUntil (1);

        // Cursor
        juce::var cur;   // null = nicht sichtbar
        auto mk = [] (float x, float y, bool pr) { juce::Array<juce::var> a { x, y, pr ? 1 : 0 }; return juce::var (a); };
        auto lerpPt = [] (juce::Point<float> a, juce::Point<float> b, double u) { return a + (b - a) * (float) ease (u); };
        const juce::Point<float> rest (ed.getWidth() * 0.82f, ed.getHeight() * 0.55f);
        const auto shapeY = (float) ed.shapeSlider.getBounds().getCentreY();
        const auto mixY = (float) ed.mixSlider.getBounds().getCentreY();
        juce::Point<float> c; bool pressed = false; bool show = true;
        if (t < 1.8)                    show = false;
        else if (t < 2.7)               c = lerpPt (rest, calC, (t - 1.8) / 0.9);
        else if (t < 3.0)               { c = calC; pressed = true; }
        else if (t < 5.9)               c = calC;
        else if (t < 6.4)               c = lerpPt (calC, knobC, (t - 5.9) / 0.5);
        else if (t < 16.0)              { c = knobC.translated ((float) (t < 11 ? ramp (t, 6.4, 10.8, 0.0, 22.0) : ramp (t, 11.2, 16.0, 22.0, 60.0)), (float) (t < 11 ? ramp (t, 6.4, 10.8, 0.0, -26.0) : ramp (t, 11.2, 16.0, -26.0, -60.0))); pressed = true; }
        else if (t < 17.2)              c = lerpPt (knobC.translated (60.0f, -60.0f), juce::Point<float> (thumbX (ed.shapeSlider, 0.5), shapeY), (t - 16.0) / 1.2);
        else if (t < 21.0)              { c = juce::Point<float> (thumbX (ed.shapeSlider, v[1]), shapeY); pressed = true; }
        else if (t < 22.5)              c = lerpPt (juce::Point<float> (thumbX (ed.shapeSlider, 0.5), shapeY), juce::Point<float> (thumbX (ed.mixSlider, 1.0), mixY), (t - 21.0) / 1.5);
        else if (t < 25.8)              { c = juce::Point<float> (thumbX (ed.mixSlider, v[2]), mixY); pressed = t >= 23.0; }
        else show = false;
        if (show) cur = mk (c.x, c.y, pressed);
        cursor.add (cur);

        if (t < GUI_UNTIL)
        {
            ReelAccess::tick (ed);
            auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
            auto file = outDir.getChildFile ("frames").getChildFile (juce::String (f).paddedLeft ('0', 4) + ".jpg");
            file.deleteFile();
            juce::FileOutputStream os (file);
            juce::JPEGImageFormat jpg; jpg.setQuality (0.93f);
            jpg.writeImageToStream (img, os);
        }
        if (f % 150 == 0) std::printf ("frame %d / %d (clip %.2f)\n", f, numFrames, v[0]);
    }

    // Ausblenden, auf -1 dBFS normalisieren
    const int fadeOut = (int) (1.6 * SR);
    for (int c = 0; c < 2; ++c) { out.applyGainRamp (c, 0, (int) (0.05 * SR), 0.0f, 1.0f); out.applyGainRamp (c, totalSamples - fadeOut, fadeOut, 1.0f, 0.0f); }
    const float pk = out.getMagnitude (0, totalSamples);
    if (pk > 0.0f) out.applyGain (0.89f / pk);
    { auto wav = outDir.getChildFile ("audio.wav"); wav.deleteFile();
      juce::WavAudioFormat fmt;
      std::unique_ptr<juce::AudioFormatWriter> wr (fmt.createWriterFor (new juce::FileOutputStream (wav), SR, 2, 24, {}, 0));
      wr->writeFromAudioSampleBuffer (out, 0, totalSamples); }

    auto* root = new juce::DynamicObject();
    root->setProperty ("fps", FPS);
    root->setProperty ("total", TOTAL);
    root->setProperty ("guiUntil", GUI_UNTIL);
    root->setProperty ("rects", juce::var (lay));
    root->setProperty ("cursor", cursor);
    outDir.getChildFile ("events.json").replaceWithText (juce::JSON::toString (juce::var (root)));
    std::printf ("done\n");
    return 0;
}
