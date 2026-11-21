#pragma once

#include "PluginProcessor.h"
#include "Look.h"
#include <juce_audio_utils/juce_audio_utils.h>

class KnackEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KnackEditor (KnackProcessor&);
    ~KnackEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::Rectangle<int> R (float x, float y, float w, float h) const;

    static constexpr float designW = 300.0f, designH = 740.0f;

    KnackProcessor& processor;
    KnackLookAndFeel lnf;

    ScopeView scope;
    juce::Slider clipKnob, inputKnob, shapeSlider, heatSlider, mixSlider, outSlider;
    juce::TextButton calibrateBtn { "CALIBRATE" };
    juce::ToggleButton hqBtn;
    VMeter inMeter { VMeter::level, KnackColours::calm };
    VMeter limMeter { VMeter::reduction, KnackColours::outCol };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> clipAtt, inputAtt, shapeAtt, heatAtt, mixAtt, outAtt;
    std::unique_ptr<BA> hqAtt;

    float glow = 0.0f, pulse = 0.0f, scale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnackEditor)
};
