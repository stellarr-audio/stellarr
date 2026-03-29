#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "blocks/Block.h"

class StellarrProcessor final : public juce::AudioProcessor
{
public:
    StellarrProcessor();
    ~StellarrProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Graph management
    juce::AudioProcessorGraph::NodeID addBlock(std::unique_ptr<stellarr::Block> block);
    void removeBlock(juce::AudioProcessorGraph::NodeID nodeId);

    bool connectBlocks(juce::AudioProcessorGraph::NodeID source,
                       juce::AudioProcessorGraph::NodeID dest,
                       int numChannels = 2);

    void disconnectBlocks(juce::AudioProcessorGraph::NodeID source,
                          juce::AudioProcessorGraph::NodeID dest);

    juce::AudioProcessorGraph& getGraph() { return graph; }

    juce::AudioProcessorGraph::NodeID getAudioInputNodeId() const  { return audioInputNodeId; }
    juce::AudioProcessorGraph::NodeID getAudioOutputNodeId() const { return audioOutputNodeId; }

private:
    juce::AudioProcessorGraph graph;

    juce::AudioProcessorGraph::NodeID audioInputNodeId;
    juce::AudioProcessorGraph::NodeID audioOutputNodeId;
};
