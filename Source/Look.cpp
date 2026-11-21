#include "Look.h"
#include "Engine.h"
#include <cmath>

using namespace KnackColours;
using namespace juce;

KnackLookAndFeel::KnackLookAndFeel()
{
    setColour (Slider::textBoxTextColourId, text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, hot.withAlpha (0.4f));
    setColour (Label::textColourId, text);
    setColour (TextEditor::highlightColourId, hot.withAlpha (0.35f));
    setColour (CaretComponent::caretColourId, hot);
    setColour (TooltipWindow::backgroundColourId, card);
    setColour (TooltipWindow::textColourId, text);
}

Label* KnackLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (FontOptions (11.5f));
    l->setColour (Label::textColourId, text.withAlpha (0.9f));
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    l->setColour (TextEditor::backgroundColourId, inset);
    l->setColour (TextEditor::textColourId, text);
    l->setColour (TextEditor::outlineColourId, hot);
    return l;
}

void KnackLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, Slider& s)
{
    const bool big = (bool) s.getProperties().getWithDefault ("big", false);
    const float glow = (float) (double) s.getProperties().getWithDefault ("glow", 0.0);
    const float pulse = (float) (double) s.getProperties().getWithDefault ("pulse", 0.0);
    const bool hover = s.isMouseOverOrDragging();

    auto bounds = Rectangle<int> (x, y, w, h).toFloat();
    const float pad = big ? 8.0f : 4.0f;
    const float size = jmin (bounds.getWidth(), bounds.getHeight()) - pad * 2.0f;
    auto r = Rectangle<float> (size, size).withCentre (bounds.getCentre());
    const auto c = r.getCentre();
    const float radius = size * 0.5f;
    const float lineW = jmax (3.0f, size * (big ? 0.05f : 0.075f));
    const float arcR = radius - lineW * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Farbe: normaler Knob folgt seinem Wert nicht, der grosse wird mit CLIP roeter
    const Colour col = big ? clipColour (glow) : (s.getProperties().contains ("accent")
                         ? Colour::fromString (s.getProperties()["accent"].toString()) : calm);

    if (big)
    {
        // Halo um den ganzen Knob: je mehr Clip, desto staerker
        const float haloA = 0.04f + 0.55f * glow + 0.12f * pulse;
        for (int i = 0; i < 5; ++i)
        {
            const float grow = (float) i * size * 0.022f;
            g.setColour (col.withAlpha (haloA * (0.22f - (float) i * 0.04f)));
            g.drawEllipse (r.expanded (grow + 2.0f), lineW * 0.9f);
        }
    }

    // Track
    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (knobTrack);
    g.strokePath (track, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));

    // Wertring
    if (pos > 0.002f)
    {
        Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (col.withAlpha (big ? 0.18f + 0.3f * glow : 0.25f));
        g.strokePath (val, PathStrokeType (lineW * (big ? 3.4f : 2.4f), PathStrokeType::curved, PathStrokeType::rounded));
        if (big)
        {
            g.setColour (col.withAlpha (0.25f + 0.3f * glow));
            g.strokePath (val, PathStrokeType (lineW * 2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (col);
        g.strokePath (val, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Koerper
    const float bodyR = arcR - lineW * 1.7f;
    auto body = Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c);
    g.setGradientFill (ColourGradient (knobBody.brighter (hover ? 0.3f : 0.18f), body.getX(), body.getY(),
                                       knobBody.darker (0.45f), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);

    if (big)
    {
        // Von innen "belichtet": roter Schein, der mit CLIP waechst
        const float in = 0.04f + 0.7f * glow + 0.1f * pulse;
        g.setGradientFill (ColourGradient (col.withAlpha (0.0f), c.x, c.y,
                                           col.withAlpha (jmin (0.85f, in)), c.x + bodyR, c.y, true));
        g.fillEllipse (body);
    }
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    // Zeiger
    const float px = c.x + std::sin (angle) * bodyR * 0.78f;
    const float py = c.y - std::cos (angle) * bodyR * 0.78f;
    const float qx = c.x + std::sin (angle) * bodyR * 0.30f;
    const float qy = c.y - std::cos (angle) * bodyR * 0.30f;
    if (big)
    {
        g.setColour (col.withAlpha (0.35f + 0.3f * glow));
        g.drawLine (qx, qy, px, py, 7.0f);
    }
    g.setColour (text);
    g.drawLine (qx, qy, px, py, big ? 3.2f : 2.2f);
}

void KnackLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         Slider::SliderStyle style, Slider& s)
{
    const auto col = Colour::fromString (s.getProperties().getWithDefault ("accent", calm.toString()).toString());
    const bool vertical = style == Slider::LinearVertical;
    const bool hover = s.isMouseOverOrDragging();

    if (! vertical)
    {
        const float cy = (float) y + (float) h * 0.5f;
        const float th = 5.0f;
        auto track = Rectangle<float> ((float) x, cy - th * 0.5f, (float) w, th);
        g.setColour (knobTrack);
        g.fillRoundedRectangle (track, th * 0.5f);
        auto fill = track.withRight (jmax (track.getX() + th, pos));
        g.setColour (col.withAlpha (0.25f));
        g.fillRoundedRectangle (fill.expanded (0.0f, 3.0f), th);
        g.setColour (col.withAlpha (hover ? 1.0f : 0.9f));
        g.fillRoundedRectangle (fill, th * 0.5f);
        g.setColour (text);
        g.fillRoundedRectangle (Rectangle<float> (5.0f, 16.0f).withCentre ({ pos, cy }), 2.5f);
    }
    else
    {
        const float cx = (float) x + (float) w * 0.5f;
        const float tw = 5.0f;
        auto track = Rectangle<float> (cx - tw * 0.5f, (float) y, tw, (float) h);
        g.setColour (knobTrack);
        g.fillRoundedRectangle (track, tw * 0.5f);
        // gefuellt von oben (0 dB) bis zur Position: so sieht man, wie weit der Ceiling abgesenkt ist
        auto fill = track.withBottom (jmax (track.getY() + tw, pos));
        g.setColour (col.withAlpha (hover ? 1.0f : 0.9f));
        g.fillRoundedRectangle (fill, tw * 0.5f);
        g.setColour (text);
        g.fillRoundedRectangle (Rectangle<float> (16.0f, 5.0f).withCentre ({ cx, pos }), 2.5f);
    }
}

void KnackLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool calibrating = (bool) b.getProperties().getWithDefault ("calibrating", false);
    const float progress = (float) (double) b.getProperties().getWithDefault ("progress", 0.0);

    g.setColour (down ? cardBorder.brighter (0.2f) : highlighted ? cardBorder : inset);
    g.fillRoundedRectangle (r, 6.0f);
    if (calibrating)
    {
        g.setColour (calm.withAlpha (0.35f));
        g.fillRoundedRectangle (r.withWidth (jmax (12.0f, r.getWidth() * progress)), 6.0f);
    }
    g.setColour (calibrating ? calm : cardBorder.brighter (0.15f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
}

void KnackLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setColour (b.getProperties().getWithDefault ("calibrating", false) ? text : text.withAlpha (0.9f));
    g.setFont (FontOptions ((float) jmin (13, b.getHeight() - 10), Font::bold));
    g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred);
}

void KnackLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const float glow = (float) (double) b.getProperties().getWithDefault ("glow", 0.0);
    const auto col = clipColour (glow);
    g.setColour (on ? col.withAlpha (0.18f) : inset);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (on ? col : (highlighted ? textDim : cardBorder.brighter (0.2f)));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
    const float d = r.getHeight() * 0.32f;
    g.setColour (on ? col : textDim.withAlpha (0.5f));
    g.fillEllipse (Rectangle<float> (d, d).withCentre ({ r.getX() + r.getHeight() * 0.55f, r.getCentreY() }));
    g.setColour (on ? text : textDim);
    g.setFont (FontOptions (r.getHeight() * 0.5f, Font::bold));
    g.drawText ("HQ", r.withTrimmedLeft (r.getHeight() * 0.8f), Justification::centred);
}

//==============================================================================
void VMeter::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (inset);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (cardBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);

    auto bar = r.reduced (1.5f);
    if (mode == reduction)
    {
        const float frac = jlimit (0.0f, 1.0f, value / 12.0f);
        g.setColour (colour);
        g.fillRoundedRectangle (bar.withHeight (jmax (0.0f, bar.getHeight() * frac)), 2.0f);
    }
    else
    {
        const float db = Decibels::gainToDecibels (value, -48.0f);
        const float frac = jlimit (0.0f, 1.0f, (db + 48.0f) / 54.0f);   // -48 .. +6 dB
        auto fill = bar.withTrimmedTop (bar.getHeight() * (1.0f - frac));
        g.setGradientFill (ColourGradient (value > 1.0f ? hot : colour, 0.0f, bar.getY(),
                                           colour.darker (0.5f), 0.0f, bar.getBottom(), false));
        g.fillRoundedRectangle (fill, 2.0f);
        const float y0 = bar.getBottom() - bar.getHeight() * (48.0f / 54.0f);   // 0-dB-Marke
        g.setColour (Colours::white.withAlpha (0.4f));
        g.drawHorizontalLine ((int) y0, bar.getX(), bar.getRight());
    }
}

//==============================================================================
static float displayWave (float s, float clip, float shape)
{
    // Anzeige, kein echtes Signal: bei 0 % reiner Sinus, bei 100 % fast ein Rechteck
    const float vd = 1.0f + 18.0f * std::pow (clip, 1.1f);
    // nur Anzeige (nicht das echte Audio): Soft = runde Welle, Hard = flaches Dach mit scharfen Ecken
    const float k = 1.05f + 45.0f * std::pow (shape, 1.6f);
    const float shaped = KnackDSP::shape (s * vd, k) / KnackDSP::shape (vd, k);
    const float w = jmin (1.0f, clip * 4.0f);
    return s + w * (shaped - s);
}

void ScopeView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const float corner = r.getHeight() * 0.14f;
    const auto col = clipColour (glow);

    g.setGradientFill (ColourGradient (inset.brighter (0.05f), r.getX(), r.getY(), inset.darker (0.3f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    // Roter Schein von innen
    g.setGradientFill (ColourGradient (col.withAlpha (0.04f + 0.30f * glow), r.getCentreX(), r.getCentreY(),
                                       col.withAlpha (0.0f), r.getX(), r.getY(), true));
    g.fillRoundedRectangle (r, corner);

    g.saveState();
    g.reduceClipRegion (r.toNearestInt());

    const float cy = r.getCentreY();
    const float amp = r.getHeight() * 0.34f;
    g.setColour (Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine ((int) cy, r.getX() + 8.0f, r.getRight() - 8.0f);

    auto buildPath = [&] (bool shaped)
    {
        Path p;
        const int n = (int) r.getWidth();
        for (int i = 0; i <= n; ++i)
        {
            const float t = (float) i / (float) n * MathConstants<float>::twoPi * 2.0f;
            const float sn = std::sin (t);
            const float v = shaped ? displayWave (sn, clip, shape) : sn;
            const float px = r.getX() + 10.0f + (float) i / (float) n * (r.getWidth() - 20.0f);
            if (i == 0) p.startNewSubPath (px, cy - v * amp); else p.lineTo (px, cy - v * amp);
        }
        return p;
    };

    // Referenz: der unbearbeitete Sinus, ganz schwach
    g.setColour (Colours::white.withAlpha (0.10f));
    g.strokePath (buildPath (false), PathStrokeType (1.0f));

    const auto wave = buildPath (true);
    g.setColour (col.withAlpha (0.10f + 0.18f * glow));
    g.strokePath (wave, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (col.withAlpha (0.25f + 0.25f * glow));
    g.strokePath (wave, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (col.brighter (0.25f));
    g.strokePath (wave, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.restoreState();

    g.setColour (textDim);
    g.setFont (FontOptions (r.getHeight() * 0.1f, Font::bold));
    g.drawText (gr > 0.1f ? "GR -" + String (gr, 1) + " dB" : "GR 0.0 dB", r.reduced (10.0f, 6.0f), Justification::bottomLeft);
    g.drawText (String (roundToInt (clip * 100.0f)) + " %", r.reduced (10.0f, 6.0f), Justification::bottomRight);
}
