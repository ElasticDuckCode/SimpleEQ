/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider {
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox) {
    }
    
};

// Isolated response curve component
struct ResponseCurve : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer {
    ResponseCurve(SimpleEQAudioProcessor&);
    ~ResponseCurve();
    
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    void timerCallback() override;
    
    void paint (juce::Graphics&) override;

private:
    SimpleEQAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> firstDraw {true};
    juce::Atomic<bool> parametersChanged {false};
    
    MonoChain monoChain;
};

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;
    
    ResponseCurve responseCurve;
    CustomRotarySlider peakFreqSlider;
    CustomRotarySlider peakGainSlider;
    CustomRotarySlider peakQualitySlider;
    CustomRotarySlider lowCutFreqSlider;
    CustomRotarySlider highCutFreqSlider;
    CustomRotarySlider lowCutSlopeSlider;
    CustomRotarySlider highCutSlopeSlider;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachement = APVTS::SliderAttachment;
    Attachement peakFreqSliderAttachment;
    Attachement peakGainSliderAttachment;
    Attachement peakQualitySliderAttachment;
    Attachement lowCutFreqSliderAttachment;
    Attachement highCutFreqSliderAttachment;
    Attachement lowCutSlopeSliderAttachment;
    Attachement highCutSlopeSliderAttachment;
    
    std::vector<juce::Component*> getComponents();
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
