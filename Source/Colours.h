#pragma once
#include <juce_graphics/juce_graphics.h>

namespace KnackColours
{
    const juce::Colour bgTop      { 0xff17181c };
    const juce::Colour bgBottom   { 0xff0d0e11 };
    const juce::Colour card       { 0xff1d1f24 };
    const juce::Colour cardBorder { 0xff2b2e35 };
    const juce::Colour inset      { 0xff121317 };
    const juce::Colour text       { 0xffeceef2 };
    const juce::Colour textDim    { 0xff7d828c };
    const juce::Colour knobTrack  { 0xff2c2f36 };
    const juce::Colour knobBody   { 0xff26282e };
    const juce::Colour calm       { 0xffff9a4d };   // wenig Clip: warmes Orange
    const juce::Colour hot        { 0xffff1f3f };   // viel Clip: Rot
    const juce::Colour heat       { 0xffffb347 };   // Heat-Regler: Amber
    const juce::Colour mixCol     { 0xff4dd0e1 };   // Mix-Regler: Tuerkis
    const juce::Colour outCol     { 0xff9fa8da };   // Output/Limiter: Lavendel

    inline juce::Colour clipColour (float g) { return calm.interpolatedWith (hot, juce::jlimit (0.0f, 1.0f, g)); }
}
