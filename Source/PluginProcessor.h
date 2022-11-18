/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


// Jake: Enum representing slope order
enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// Jake: Struct for extracting settings from our parameters
struct ChainSettings {
    float peakFreq = 0;
    float peakGainInDecibels = 0;
    float peakQuality = 0;
    float lowCutFreq = 0;
    float highCutFreq = 0;
    Slope lowCutSlope = Slope::Slope_12;
    Slope highCutSlope = Slope::Slope_12;
};
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);


//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
   
    // Jake: Needed to syncrhonize parameters among differ components of audio plugin
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState aptvs{*this, nullptr, "Parameters", createParameterLayout()};
    
private:
    //== Setting Aliases ===========================================================
    
    using Filter = juce::dsp::IIR::Filter<float>;
    
    //NOTE: Processor chains allows running of 4 filters in a single chain call
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    MonoChain leftChain;
    MonoChain rightChain;
    
    enum ChainPositions {
        LowCut,
        Peak,
        HighCut
    };
    
    using Coefficients = Filter::CoefficientsPtr;
    void updatePeakFilter(const ChainSettings& chainSettings);
    static void updateCoefficents(Coefficients& old, const Coefficients& replacement);
    
    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& cutChain, const CoefficientType lowCutCoefficients, const Slope& lowCutSlope) {
        
        cutChain.template setBypassed<0>(true);
        cutChain.template setBypassed<1>(true);
        cutChain.template setBypassed<2>(true);
        cutChain.template setBypassed<3>(true);
        switch (lowCutSlope) {
            case Slope_12:
                *cutChain.template get<0>().coefficients = *lowCutCoefficients[0];
                cutChain.template setBypassed<0>(false);
                break;
            case Slope_24:
                *cutChain.template get<0>().coefficients = *lowCutCoefficients[0];
                cutChain.template setBypassed<0>(false);
                *cutChain.template get<1>().coefficients = *lowCutCoefficients[1];
                cutChain.template setBypassed<1>(false);
                break;
            case Slope_36:
                *cutChain.template get<0>().coefficients = *lowCutCoefficients[0];
                cutChain.template setBypassed<0>(false);
                *cutChain.template get<1>().coefficients = *lowCutCoefficients[1];
                cutChain.template setBypassed<1>(false);
                *cutChain.template get<2>().coefficients = *lowCutCoefficients[2];
                cutChain.template setBypassed<2>(false);
                break;
            case Slope_48:
                *cutChain.template get<3>().coefficients = *lowCutCoefficients[3];
                cutChain.template setBypassed<3>(false);
                *cutChain.template get<0>().coefficients = *lowCutCoefficients[0];
                cutChain.template setBypassed<0>(false);
                *cutChain.template get<1>().coefficients = *lowCutCoefficients[1];
                cutChain.template setBypassed<1>(false);
                *cutChain.template get<2>().coefficients = *lowCutCoefficients[2];
                cutChain.template setBypassed<2>(false);
                break;
    }
    
    }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
