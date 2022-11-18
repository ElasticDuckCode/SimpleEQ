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
    
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->addListener(this);
    }
    
    startTimer(60);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->removeListener(this);
    }
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    
    // Draw Magnitude Response
    g.fillAll(juce::Colours::black);
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 1/3);
    auto& lowCut = monoChain.get<ChainPositions::LowCut>();
    auto& highCut = monoChain.get<ChainPositions::HighCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    mags.resize(responseArea.getWidth());
    
    auto W = responseArea.getWidth();
    for (int i = 0; i < W; i++) {
        double mag = 1.0f;
        auto freq = juce::mapToLog10(double(i) / W, 20.0, 20000.0);
        
        // Get magnitude by multiplying value for all filters
        if (!monoChain.isBypassed<ChainPositions::Peak>()) {
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        if (!lowCut.isBypassed<0>()) {
            mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowCut.isBypassed<1>()) {
            mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowCut.isBypassed<2>()) {
            mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowCut.isBypassed<3>()) {
            mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        if (!highCut.isBypassed<0>()) {
            mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highCut.isBypassed<1>()) {
            mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highCut.isBypassed<2>()) {
            mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highCut.isBypassed<3>()) {
            mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        mags[i] = juce::Decibels::gainToDecibels(mag);
    }
    
    juce::Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input) {
        return juce::jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for (size_t i = 0; i < mags.size(); ++i) {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
        
    }
    
    g.setColour(juce::Colours::blue);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.0, 1.0);
    g.setColour(juce::Colours::yellow);
    g.strokePath(responseCurve, juce::PathStrokeType(2.0));
    
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

void SimpleEQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged = true;
    
}

void SimpleEQAudioProcessorEditor::timerCallback() {
    if (parametersChanged.compareAndSetBool(false, true)) {
        //update monochain
        auto chainSettings = getChainSettings(audioProcessor.aptvs);
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficents(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        //signal repaint
    }
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
