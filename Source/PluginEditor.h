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

struct LookAndFeel : juce::LookAndFeel_V4 {
    // Look and feel class for RotarySliderWithLabels
    void drawRotarySlider (juce::Graphics&,
                           int x, int y, int width, int height,
                           float sliderposproportional,
                           float rotarystartangle,
                           float rotaryendangle,
                           juce::Slider&) override;

};

struct RotarySliderWithLabels : juce::Slider {
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String unitSuffix) :juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox), param(&rap), suffix(unitSuffix) {
        setLookAndFeel(&lnf);
    }
    ~RotarySliderWithLabels() {
        setLookAndFeel(nullptr);
    }
    
    struct Labels {
        float pos;
        juce::String label;
    };
    juce::Array<Labels> labels;
    
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
    
private:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};

// Isolated response curve component
struct ResponseCurve : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer {
    ResponseCurve(SimpleEQAudioProcessor&);
    ~ResponseCurve();
    
    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    void timerCallback() override;
    
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SimpleEQAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged {false};
    juce::Image background;
    
    MonoChain monoChain;
    void updateChain();
    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysisArea();
    int getTextHeight() const { return 12; }
    
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
    RotarySliderWithLabels peakFreqSlider;
    RotarySliderWithLabels peakGainSlider;
    RotarySliderWithLabels peakQualitySlider;
    RotarySliderWithLabels lowCutFreqSlider;
    RotarySliderWithLabels highCutFreqSlider;
    RotarySliderWithLabels lowCutSlopeSlider;
    RotarySliderWithLabels highCutSlopeSlider;
    
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
