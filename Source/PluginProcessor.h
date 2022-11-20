/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>


enum Channel {
    Right,
    Left
};


template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert( std::is_same_v<T, juce::AudioBuffer<float>>,
                      "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for( auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                           numSamples,
                           false,   //clear everything?
                           true,    //including the extra space?
                           true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }
    
    void prepare(size_t numElements)
    {
        static_assert( std::is_same_v<T, std::vector<float>>,
                      "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for( auto& buffer : buffers )
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }
    
    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if( write.blockSize1 > 0 )
        {
            buffers[write.startIndex1] = t;
            return true;
        }
        
        return false;
    }
    
    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if( read.blockSize1 > 0 )
        {
            t = buffers[read.startIndex1];
            return true;
        }
        
        return false;
    }
    
    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo {Capacity};
};


template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }
    
    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse );
        auto* channelPtr = buffer.getReadPointer(channelToUse);
        
        for( int i = 0; i < buffer.getNumSamples(); ++i )
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);
        
        bufferToFill.setSize(1,             //channel
                             bufferSize,    //num samples
                             false,         //keepExistingContent
                             true,          //clear extra space
                             true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;
    
    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);
            
            fifoIndex = 0;
        }
        
        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};


// Jake: Enum representing slope order
enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// Jake: Names of filters and chains used in project.
using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
using Coefficients = Filter::CoefficientsPtr;
using CoefficientArray = juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>>;


// Jake: Enum representing positions of filters in chain.
enum ChainPositions {
    LowCut,
    Peak,
    HighCut
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

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);
CoefficientArray makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate);
CoefficientArray makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate);
void updateCoefficents(Coefficients& old, const Coefficients& replacement);
template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& cutChain, const CoefficientType cutCoefficients, const Slope& cutSlope) {
    cutChain.template setBypassed<0>(true);
    cutChain.template setBypassed<1>(true);
    cutChain.template setBypassed<2>(true);
    cutChain.template setBypassed<3>(true);
    switch (cutSlope) {
        case Slope_48:
            updateCoefficents(cutChain.template get<3>().coefficients, cutCoefficients[3]);
            cutChain.template setBypassed<3>(false);
        case Slope_36:
            updateCoefficents(cutChain.template get<2>().coefficients, cutCoefficients[2]);
            cutChain.template setBypassed<2>(false);
        case Slope_24:
            updateCoefficents(cutChain.template get<1>().coefficients, cutCoefficients[1]);
            cutChain.template setBypassed<1>(false);
        case Slope_12:
            updateCoefficents(cutChain.template get<0>().coefficients, cutCoefficients[0]);
            cutChain.template setBypassed<0>(false);
            break;
    }
}
    


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
    
    //NOTE: Processor chains allows running of 4 filters in a single chain call
    MonoChain leftChain;
    MonoChain rightChain;
    
    void updatePeakFilter(const ChainSettings& chainSettings);
    void updateLowCutFilter(const ChainSettings& chainSettings);
    void updateHighCutFilter(const ChainSettings& chainSettings);
    void updateFilters();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
