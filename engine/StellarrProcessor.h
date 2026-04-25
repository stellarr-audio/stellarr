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
    using UpdateKind = juce::AudioProcessorGraph::UpdateKind;

    juce::AudioProcessorGraph::NodeID addBlock(
        std::unique_ptr<stellarr::Block> block,
        UpdateKind update = UpdateKind::sync);

    void removeBlock(juce::AudioProcessorGraph::NodeID nodeId,
                     UpdateKind update = UpdateKind::sync);

    bool connectBlocks(juce::AudioProcessorGraph::NodeID source,
                       juce::AudioProcessorGraph::NodeID dest,
                       int numChannels = 2,
                       UpdateKind update = UpdateKind::sync);

    void disconnectBlocks(juce::AudioProcessorGraph::NodeID source,
                          juce::AudioProcessorGraph::NodeID dest,
                          UpdateKind update = UpdateKind::sync);

    void rebuildGraph() { graph.rebuild(); }

    juce::AudioProcessorGraph& getGraph() { return graph; }

    /**
     * RAII helper for user-initiated batches of graph mutations. On
     * construction, suspends top-level processing so the audio thread is
     * synchronised. On destruction, performs a single rebuildGraph() and
     * resumes processing. Pass UpdateKind::none to all graph calls within
     * the scope so the dtor's rebuild is the only rebuild for the batch.
     *
     * Slow work (plugin loading, etc.) should still happen BEFORE entering
     * the scope to keep the suspension window minimal.
     */
    class GraphMutationScope
    {
    public:
        explicit GraphMutationScope(StellarrProcessor& p) : processor(p)
        {
            processor.suspendProcessing(true);
        }

        ~GraphMutationScope()
        {
            processor.rebuildGraph();
            processor.suspendProcessing(false);
        }

        GraphMutationScope(const GraphMutationScope&) = delete;
        GraphMutationScope& operator=(const GraphMutationScope&) = delete;

    private:
        StellarrProcessor& processor;
    };

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
