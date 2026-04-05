#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "blocks/Block.h"
#include "PluginManager.h"
#include "MidiMapper.h"

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
    juce::AudioProcessorGraph::NodeID getMidiInputNodeId() const   { return midiInputNodeId; }
    juce::AudioProcessorGraph::NodeID getMidiOutputNodeId() const  { return midiOutputNodeId; }

    PluginManager& getPluginManager() { return pluginManager; }
    MidiMapper& getMidiMapper() { return midiMapper; }

    double getCpuUsagePercent() const { return cpuUsagePercent.load(std::memory_order_relaxed); }
    float getOutputPeakLevel() const { return outputPeakLevel.load(std::memory_order_relaxed); }

    void setAppProperties(juce::ApplicationProperties* props) { appProperties = props; }
    juce::ApplicationProperties* getAppProperties() const { return appProperties; }

private:
    juce::AudioProcessorGraph graph;

    juce::AudioProcessorGraph::NodeID audioInputNodeId;
    juce::AudioProcessorGraph::NodeID audioOutputNodeId;
    juce::AudioProcessorGraph::NodeID midiInputNodeId;
    juce::AudioProcessorGraph::NodeID midiOutputNodeId;

    PluginManager pluginManager;
    MidiMapper midiMapper;
    juce::ApplicationProperties* appProperties = nullptr;
    std::atomic<double> cpuUsagePercent { 0.0 };
    std::atomic<float> outputPeakLevel { 0.0f };
};
