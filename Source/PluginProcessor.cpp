#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout KnackProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto dbText = [] (float v, int) { return String (v, 1) + " dB"; };
    auto pctText = [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; };

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::input, 1 }, "Input",
                    NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::clip, 1 }, "Clip",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f, AudioParameterFloatAttributes().withStringFromValueFunction (pctText)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::shape, 1 }, "Shape",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v < 0.02f ? String ("Soft") : v > 0.98f ? String ("Hard") : String (roundToInt (v * 100.0f)) + " %"; })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::mix, 1 }, "Mix",
                    NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, AudioParameterFloatAttributes().withStringFromValueFunction (pctText)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::output, 1 }, "Output",
                    NormalisableRange<float> (-12.0f, 0.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::hq, 1 }, "HQ", true));

    return { p.begin(), p.end() };
}

KnackProcessor::KnackProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KNACK", createLayout())
{
    pInput = apvts.getRawParameterValue (ParamID::input);
    pClip  = apvts.getRawParameterValue (ParamID::clip);
    pShape = apvts.getRawParameterValue (ParamID::shape);
    pMix   = apvts.getRawParameterValue (ParamID::mix);
    pOut   = apvts.getRawParameterValue (ParamID::output);
    pHq    = apvts.getRawParameterValue (ParamID::hq);
}

bool KnackProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return (o == AudioChannelSet::stereo() || o == AudioChannelSet::mono()) && o == l.getMainInputChannelSet();
}

void KnackProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
    lastHq = pHq->load() > 0.5f;
    setLatencySamples (engine.getLatencySamples (lastHq));
}

void KnackProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    KnackParams p;
    p.inputDb  = pInput->load();
    p.clip     = pClip->load();
    p.shape    = pShape->load();
    p.mix      = pMix->load();
    p.outputDb = pOut->load();
    p.hq       = pHq->load() > 0.5f;

    if (p.hq != lastHq)
    {
        lastHq = p.hq;
        setLatencySamples (engine.getLatencySamples (lastHq));
    }

    engine.process (buffer, p);

    float db;
    if (engine.popCalibrationResult (db))
    {
        pendingInputDb = db;
        triggerAsyncUpdate();
    }
}

void KnackProcessor::handleAsyncUpdate()
{
    // Calibrate fertig: Input-Regler auf den gemessenen Wert setzen (Host sieht es als Automation)
    if (auto* par = apvts.getParameter (ParamID::input))
    {
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 (pendingInputDb.load()));
        par->endChangeGesture();
    }
}

AudioProcessorEditor* KnackProcessor::createEditor() { return new KnackEditor (*this); }

void KnackProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void KnackProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KnackProcessor(); }
