#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"

namespace ParamID
{
    inline constexpr const char* input  = "input";
    inline constexpr const char* clip   = "clip";
    inline constexpr const char* shape  = "shape";
    inline constexpr const char* heat   = "heat";
    inline constexpr const char* mix    = "mix";
    inline constexpr const char* output = "output";
    inline constexpr const char* hq     = "hq";
}

class KnackProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    KnackProcessor();
    ~KnackProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "KNACK"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    KnackEngine& getEngine() { return engine; }
    juce::AudioProcessorValueTreeState apvts;

private:
    void handleAsyncUpdate() override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    KnackEngine engine;
    std::atomic<float>* pInput = nullptr; std::atomic<float>* pClip = nullptr; std::atomic<float>* pShape = nullptr; std::atomic<float>* pHeat = nullptr;
    std::atomic<float>* pMix = nullptr;   std::atomic<float>* pOut = nullptr;  std::atomic<float>* pHq = nullptr;
    std::atomic<float> pendingInputDb { 0.0f };
    bool lastHq = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnackProcessor)
};
