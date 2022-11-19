/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) {
    auto bounds = juce::Rectangle<float>(x,  y, width, height);
    
    // draw rotary knob
    g.setColour(juce::Colours::blue);
    g.fillEllipse(bounds);
    g.setColour(juce::Colours::yellow);
    g.drawEllipse(bounds, 1.0);
    
    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider)) {
        // draw rotary notch
        auto center = bounds.getCentre();
        juce::Path path;
        juce::Rectangle<float> rect;
        rect.setLeft(center.getX() - 2);
        rect.setRight(center.getX() + 2);
        rect.setTop(bounds.getY());
        rect.setBottom(center.getY() - 1.5 * rswl->getTextHeight());
        path.addRectangle(rect);
        jassert(rotaryStartAngle < rotaryEndAngle);
        auto sliderAngleInRadians = juce::jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        path.applyTransform(juce::AffineTransform().rotation(sliderAngleInRadians, center.getX(), center.getY()));
        g.fillPath(path);
        
        // draw rotary text
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        rect.setSize(strWidth + 4, rswl->getTextHeight());
        rect.setCentre(center);
        //g.seColour(juce::Colours::black);
        //g.fillRect(rect);
        g.setColour(juce::Colours::yellow);
        g.drawFittedText(text, rect.toNearestInt(), juce::Justification::centred, 1);
    }
    return;
}
//==============================================================================
juce::String RotarySliderWithLabels::getDisplayString() const {
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();
    
    juce::String string;
    bool kHz = false;
    float value = (float)getValue();
    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
        if (value >= 1000.0) {
            value /= 1000.0;
            kHz = true;
        }
    }
    else {
        jassertfalse;
    }
    
    if (kHz) {
        string = juce::String(value, 1, false);
    }
    else {
        string = juce::String(value, 0, false);
    }
    
    if (suffix.isNotEmpty()) {
        string << " ";
        if (kHz)
            string << "k";
        string << suffix;
    }
    
    return string;
}

void RotarySliderWithLabels::paint(juce::Graphics &g) {
    auto startAngle = juce::degreesToRadians(180.0 + 45.0);
    auto endAngle = juce::degreesToRadians(180.0 - 45.0) + juce::MathConstants<float>::twoPi;
    auto range = getRange();
    auto sliderBounds = getSliderBounds();
    
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      juce::jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAngle,
                                      endAngle,
                                      *this);
    return;
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= 2 * getTextHeight();
    juce::Rectangle<int> rect;
    rect.setSize(size, size);
    rect.setCentre(bounds.getCentreX(), bounds.getCentreY());
    return rect;
}

//==============================================================================
ResponseCurve::ResponseCurve(SimpleEQAudioProcessor& p) : audioProcessor(p) {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->addListener(this);
    }
    startTimerHz(60);
    
    // Perform first chain update.
    updateChain();
}

ResponseCurve::~ResponseCurve() {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->removeListener(this);
    }
}

void ResponseCurve::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true);
}

void ResponseCurve::timerCallback() {
    if (parametersChanged.compareAndSetBool(false, true)) {
        updateChain();
        repaint();
    }
}

void ResponseCurve::updateChain() {
    auto chainSettings = getChainSettings(audioProcessor.aptvs);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficents(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
    return;
}

void ResponseCurve::paint (juce::Graphics& g)
{
    // Draw Magnitude Response
    g.fillAll(juce::Colours::black);
    
    auto responseArea = getLocalBounds();
    auto W = responseArea.getWidth();
    
    auto& lowCut = monoChain.get<ChainPositions::LowCut>();
    auto& highCut = monoChain.get<ChainPositions::HighCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    mags.resize(responseArea.getWidth());
    
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
    
    for (size_t i = 1; i < mags.size(); ++i) {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
        
    }
    g.setColour(juce::Colours::blue);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.0, 1.0);
    g.setColour(juce::Colours::yellow);
    g.strokePath(responseCurve, juce::PathStrokeType(2.0));
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      responseCurve(audioProcessor),

      peakFreqSlider(*audioProcessor.aptvs.getParameter("Peak Freq"), "Hz"),
      peakGainSlider(*audioProcessor.aptvs.getParameter("Peak Gain"), "dB"),
      peakQualitySlider(*audioProcessor.aptvs.getParameter("Peak Quality"), ""),
      lowCutFreqSlider(*audioProcessor.aptvs.getParameter("Low-Cut Freq"), "Hz"),
      highCutFreqSlider(*audioProcessor.aptvs.getParameter("High-Cut Freq"), "Hz"),
      lowCutSlopeSlider(*audioProcessor.aptvs.getParameter("Low-Cut Slope"), "dB/Oct"),
      highCutSlopeSlider(*audioProcessor.aptvs.getParameter("High-Cut Slope"), "dB/Oct"),

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
    for (auto& comp : getComponents()) {
        addAndMakeVisible(comp);
    }
    setSize (600, 500);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Draw Magnitude Response
    g.fillAll(juce::Colours::black);
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 1/2);
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 1/3);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 1/2);
    auto peakFreqArea = bounds.removeFromTop(bounds.getHeight() * 1/3);
    auto peakGainArea = bounds.removeFromTop(bounds.getHeight() * 1/2);
    
    auto lowCutFreqArea = lowCutArea.removeFromTop(lowCutArea.getHeight() * 1/2);
    auto highCutFreqArea = highCutArea.removeFromTop(highCutArea.getHeight() * 1/2);
    
    responseCurve.setBounds(responseArea);
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
        &highCutSlopeSlider,
        &responseCurve
    };
}
