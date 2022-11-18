/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    // Create ProcessSpec
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    // Private MonoChain must be prepared using our spec
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    // Get defined parameters
    auto chainSettings = getChainSettings(aptvs);
    
    // Create filters
    updateFilters();
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//        auto* channelData = buffer.getWritePointer (channel);
//
//        // ..do something to the data...
//    }
    
    // Update filters before processing audio
    updateFilters();
    
    // Process AudioBlock for Left and Right Channel
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    //return new SimpleEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//== Jake ======================================================================

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) {
    /* Extract settings from given parameters */
    ChainSettings settings;
    
    // Load defined parameters
    settings.lowCutFreq = apvts.getRawParameterValue("Low-Cut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("High-Cut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = static_cast<Slope>(apvts.getRawParameterValue("Peak Quality")->load());
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("Low-Cut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("High-Cut Slope")->load());
    
    return settings;
}

void SimpleEQAudioProcessor::updateCoefficents(Coefficients& old, const Coefficients& replacement) {
    *old = *replacement;
    return;
}

void SimpleEQAudioProcessor::updateFilters() {
    auto chainSettings = getChainSettings(aptvs);
    updatePeakFilter(chainSettings);
    updateLowCutFilter(chainSettings);
    updateHighCutFilter(chainSettings);
    return;
}

void SimpleEQAudioProcessor::updateLowCutFilter(const ChainSettings &chainSettings) {
    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, getSampleRate(),  2 * (chainSettings.lowCutSlope) + 1);
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    return;
}

void SimpleEQAudioProcessor::updateHighCutFilter(const ChainSettings &chainSettings) {
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, getSampleRate(), 2 * (chainSettings.highCutSlope + 1));
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
    return;
}

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings) {
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality, juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    updateCoefficents(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficents(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    return;
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Create parameters for each component of our audio plugin.
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"Low-Cut Freq", 1}, "Low-Cut Freq", juce::NormalisableRange<float>(20.0, 20000.0, 1.0, 0.4), 20.0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"High-Cut Freq", 1}, "High-Cut Freq", juce::NormalisableRange<float>(20.0, 20000.0, 1.0, 0.4), 20000.0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"Peak Freq", 1}, "Peak Freq", juce::NormalisableRange<float>(20.0, 20000.0, 1.0, 0.4), 750.0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"Peak Gain", 1}, "Peak Gain", juce::NormalisableRange<float>(-24.0, 24.0, 0.5, 1.0), 0.0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"Peak Quality", 1}, "Peak Quality", juce::NormalisableRange<float>(0.1, 10.0, 0.05, 1.0), 1.0));
    juce::StringArray choiceArray;
    for(int i = 0; i < 4; i++) {
        juce::String str;
        str << (12 + i*12);
        str << " db/Oct";
        choiceArray.add(str);
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"Low-Cut Slope", 1}, "Low-Cut Slope", choiceArray, 0.0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"High-Cut Slope", 1}, "High-Cut Slope", choiceArray, 0.0));
    
    return layout;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
