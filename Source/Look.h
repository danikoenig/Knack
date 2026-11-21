#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Colours.h"

// Look & Feel im Dani-Koenig-Design (wie One Shooter / Quad Shooter), Hochformat
class KnackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnackLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// Schmaler vertikaler Pegel: Eingang (Peak) oder Gain Reduction
class VMeter : public juce::Component
{
public:
    enum Mode { level, reduction };
    VMeter (Mode m, juce::Colour c) : mode (m), colour (c) {}
    void setValue (float v) { if (std::abs (v - value) > 0.002f) { value = v; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    Mode mode;
    juce::Colour colour;
    float value = 0.0f;
};

// Oben: Sinus -> immer staerker gequetscht, je weiter CLIP aufgedreht ist
class ScopeView : public juce::Component
{
public:
    void setState (float clipValue, float shapeValue, float glowValue)
    {
        clip = clipValue; shape = shapeValue; glow = glowValue; repaint();
    }
    void paint (juce::Graphics&) override;
private:
    float clip = 0.0f, shape = 0.5f, glow = 0.0f;
};
