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

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor, juce::AudioProcessorParameter::Listener, juce::Timer
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> parametersChanged = false;
    
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
    
    MonoChain monoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
