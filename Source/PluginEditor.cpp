/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      peakFreqSliderAttachment(audioProcessor.aptvs, "Peak Freq", peakFreqSlider),
      peakGainSliderAttachment(audioProcessor.aptvs, "Peak Gain", peakGainSlider),
      peakQualitySliderAttachment(audioProcessor.aptvs, "Peak Quality", peakQualitySlider),
      lowCutFreqSliderAttachment(audioProcessor.aptvs, "Low-Cut Freq", lowCutFreqSlider),
      highCutFreqSliderAttachment(audioProcessor.aptvs, "High-Cut Freq", highCutFreqSlider),
      lowCutSlopeSliderAttachment(audioProcessor.aptvs, "Low-Cut Slope", lowCutSlopeSlider),
      highCutSlopeSliderAttachment(audioProcessor.aptvs, "High-Cut Slope", highCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);
    
    for (auto& comp : getComponents()) {
        addAndMakeVisible(comp);
    }
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 1/3);
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 1/3);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 1/2);
    auto peakFreqArea = bounds.removeFromTop(bounds.getHeight() * 1/3);
    auto peakGainArea = bounds.removeFromTop(bounds.getHeight() * 1/2);
    
    auto lowCutFreqArea = lowCutArea.removeFromTop(lowCutArea.getHeight() * 1/2);
    auto highCutFreqArea = highCutArea.removeFromTop(highCutArea.getHeight() * 1/2);
    
    
    lowCutFreqSlider.setBounds(lowCutFreqArea);
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutFreqArea);
    highCutSlopeSlider.setBounds(highCutArea);
    peakFreqSlider.setBounds(peakFreqArea);
    peakGainSlider.setBounds(peakGainArea);
    peakQualitySlider.setBounds(bounds);
    
    return;
}


std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComponents() {
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
    };
}
