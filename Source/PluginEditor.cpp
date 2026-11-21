#include "PluginEditor.h"

using namespace juce;
using namespace KnackColours;

KnackEditor::KnackEditor (KnackProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    auto& apvts = processor.apvts;

    auto setupSlider = [&] (Slider& s, const char* id, Slider::SliderStyle style, Slider::TextEntryBoxPosition tb, Colour accent)
    {
        s.setSliderStyle (style);
        s.setTextBoxStyle (tb, false, 54, 16);
        s.getProperties().set ("accent", accent.toString());
        s.setMouseDragSensitivity (200);
        s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        addAndMakeVisible (s);
        auto* param = apvts.getParameter (id);
        s.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    };

    setupSlider (clipKnob,  ParamID::clip,   Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow, calm);
    clipKnob.getProperties().set ("big", true);
    setupSlider (inputKnob, ParamID::input,  Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow, calm);
    setupSlider (shapeSlider, ParamID::shape, Slider::LinearHorizontal, Slider::NoTextBox, clipColour (0.8f));
    setupSlider (mixSlider,  ParamID::mix,   Slider::LinearHorizontal, Slider::TextBoxRight, mixCol);
    setupSlider (outSlider,  ParamID::output, Slider::LinearVertical, Slider::NoTextBox, outCol);
    clipKnob.setMouseDragSensitivity (260);

    clipAtt  = std::make_unique<SA> (apvts, ParamID::clip, clipKnob);
    inputAtt = std::make_unique<SA> (apvts, ParamID::input, inputKnob);
    shapeAtt = std::make_unique<SA> (apvts, ParamID::shape, shapeSlider);
    mixAtt   = std::make_unique<SA> (apvts, ParamID::mix, mixSlider);
    outAtt   = std::make_unique<SA> (apvts, ParamID::output, outSlider);

    addAndMakeVisible (scope);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (limMeter);

    hqBtn.setButtonText ("HQ");
    addAndMakeVisible (hqBtn);
    hqAtt = std::make_unique<BA> (apvts, ParamID::hq, hqBtn);
    hqBtn.setTooltip ("High Quality: 4x oversampling");

    calibrateBtn.setTooltip ("Plays your signal for ~3 s and sets INPUT so the loudest peak lands at -6 dBFS");
    calibrateBtn.onClick = [this]
    {
        auto& e = processor.getEngine();
        if (e.isCalibrating()) e.cancelCalibration(); else e.startCalibration();
    };
    addAndMakeVisible (calibrateBtn);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);
    setResizeLimits (240, (int) (240.0f * designH / designW), 600, (int) (600.0f * designH / designW));
    setSize ((int) designW, (int) designH);

    glow = (float) clipKnob.getValue();
    startTimerHz (30);
}

KnackEditor::~KnackEditor()
{
    setLookAndFeel (nullptr);
}

Rectangle<int> KnackEditor::R (float x, float y, float w, float h) const
{
    return { roundToInt (x * scale), roundToInt (y * scale), roundToInt (w * scale), roundToInt (h * scale) };
}

void KnackEditor::resized()
{
    scale = (float) getWidth() / designW;

    hqBtn.setBounds (R (232, 672, 52, 22));
    scope.setBounds (R (16, 84, 268, 100));

    shapeSlider.setBounds (R (60, 192, 180, 22));

    clipKnob.setBounds (R (10, 218, 280, 278));
    clipKnob.setTextBoxStyle (Slider::TextBoxBelow, false, roundToInt (80 * scale), roundToInt (18 * scale));

    mixSlider.setBounds (R (66, 520, 218, 26));
    mixSlider.setTextBoxStyle (Slider::TextBoxRight, false, roundToInt (54 * scale), roundToInt (16 * scale));

    inputKnob.setBounds (R (14, 594, 84, 92));
    inputKnob.setTextBoxStyle (Slider::TextBoxBelow, false, roundToInt (70 * scale), roundToInt (16 * scale));
    inMeter.setBounds (R (106, 598, 12, 72));
    calibrateBtn.setBounds (R (16, 692, 124, 26));

    outSlider.setBounds (R (176, 598, 28, 96));
    limMeter.setBounds (R (214, 598, 12, 96));
}

void KnackEditor::timerCallback()
{
    auto& e = processor.getEngine();
    const float clip = (float) clipKnob.getValue();

    // Glow folgt dem CLIP-Wert weich; etwas Puls von der echten Gain Reduction
    glow += (clip - glow) * 0.25f;
    pulse += (jlimit (0.0f, 1.0f, e.clipGrDb.load() / 18.0f) - pulse) * 0.3f;

    clipKnob.getProperties().set ("glow", (double) glow);
    clipKnob.getProperties().set ("pulse", (double) pulse);
    hqBtn.getProperties().set ("glow", (double) glow);
    clipKnob.repaint();
    hqBtn.repaint();

    scope.setState (clip, (float) shapeSlider.getValue(), glow, e.clipGrDb.load());
    inMeter.setValue (e.inPeak.load());
    limMeter.setValue (e.limGrDb.load());

    const bool cal = e.isCalibrating();
    calibrateBtn.getProperties().set ("calibrating", cal);
    calibrateBtn.getProperties().set ("progress", (double) e.getCalProgress());
    calibrateBtn.setButtonText (cal ? "LISTENING..." : "CALIBRATE");
    calibrateBtn.repaint();

    repaint (R (0, 0, designW, 572));   // Hintergrund-Glow
    repaint (R (150, 570, 150, 160));
}

void KnackEditor::paint (Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, bounds.getBottom(), false));
    g.fillAll();

    // Roter Schein hinter dem grossen Knob: wird mit CLIP groesser und staerker
    const auto c = Point<float> (150.0f * scale, 348.0f * scale);
    const float rad = (150.0f + 330.0f * glow) * scale;
    const float a = 0.03f + 0.40f * glow + 0.08f * pulse;
    g.setGradientFill (ColourGradient (clipColour (glow).withAlpha (a), c.x, c.y,
                                       clipColour (glow).withAlpha (0.0f), c.x + rad, c.y, true));
    g.fillAll();

    // Titel
    g.setColour (text);
    g.setFont (FontOptions (38.0f * scale, Font::bold));
    {
        const String title = "KNACK";
        const float spacing = 6.0f * scale;
        GlyphArrangement ga;
        ga.addLineOfText (g.getCurrentFont(), title, 0.0f, 0.0f);
        float wTot = 0.0f;
        for (int i = 0; i < ga.getNumGlyphs(); ++i) wTot += ga.getGlyph (i).getRight() - ga.getGlyph (i).getLeft() + spacing;
        float x = ((float) getWidth() - wTot + spacing) * 0.5f;
        for (int i = 0; i < title.length(); ++i)
        {
            const auto s = String::charToString (title[i]);
            const float gw = g.getCurrentFont().getStringWidthFloat (s);
            g.drawText (s, Rectangle<float> (x, 16.0f * scale, gw + 4.0f, 44.0f * scale), Justification::centredLeft);
            x += gw + spacing;
        }
    }
    g.setColour (textDim);
    g.setFont (FontOptions (9.5f * scale, Font::bold));
    g.drawText ("by Dani Koenig", R (0, 5, designW, 12).toFloat(), Justification::centred);
    g.setColour (clipColour (glow));
    g.setFont (FontOptions (12.0f * scale, Font::bold));
    g.drawText ("C L I P P E R", R (0, 58, designW, 16).toFloat(), Justification::centred);

    // Beschriftungen
    g.setColour (textDim);
    g.setFont (FontOptions (10.5f * scale, Font::bold));
    g.drawText ("CLIP", R (0, 496, designW, 12).toFloat(), Justification::centred);
    g.drawText ("SOFT", R (16, 192, 42, 22).toFloat(), Justification::centredRight);
    g.drawText ("HARD", R (242, 192, 42, 22).toFloat(), Justification::centredLeft);
    g.drawText ("MIX", R (16, 520, 48, 26).toFloat(), Justification::centredLeft);
    g.drawText ("INPUT", R (16, 576, 100, 14).toFloat(), Justification::centredLeft);
    g.drawText ("OUTPUT", R (176, 576, 100, 14).toFloat(), Justification::centredLeft);

    // Trennlinie
    g.setColour (cardBorder);
    g.drawHorizontalLine (roundToInt (566.0f * scale), 16.0f * scale, (designW - 16.0f) * scale);

    // Output-Anzeige: 0 dB = 0 dBFS Ceiling; GR des Limiters in 0.1-dB-Schritten
    const float outDb = (float) outSlider.getValue();
    const float gr = processor.getEngine().limGrDb.load();
    g.setColour (textDim);
    g.setFont (FontOptions (9.5f * scale, Font::bold));
    g.drawText ("GR", R (232, 598, 60, 12).toFloat(), Justification::centredLeft);
    g.setColour (gr > 0.05f ? outCol : text.withAlpha (0.9f));
    g.setFont (FontOptions (13.0f * scale, Font::bold));
    g.drawText (gr > 0.05f ? "-" + String (gr, 1) : "0.0", R (232, 610, 60, 16).toFloat(), Justification::centredLeft);
    g.setColour (textDim);
    g.setFont (FontOptions (9.5f * scale, Font::bold));
    g.drawText ("LEVEL", R (232, 636, 60, 12).toFloat(), Justification::centredLeft);
    g.setColour (text.withAlpha (0.9f));
    g.setFont (FontOptions (12.0f * scale));
    g.drawText (String (outDb, 1) + " dB", R (232, 648, 60, 16).toFloat(), Justification::centredLeft);
}

void KnackEditor::paintOverChildren (Graphics& g)
{
    // Leuchtender Rahmen ums ganze Fenster, wird mit CLIP staerker
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (clipColour (glow).withAlpha (0.08f + 0.6f * glow));
    g.drawRoundedRectangle (r, 6.0f * scale, 1.5f);
}
