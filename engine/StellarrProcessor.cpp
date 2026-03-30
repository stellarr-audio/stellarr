#include "StellarrProcessor.h"
#include "StellarrEditor.h"

StellarrProcessor::StellarrProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    graph.enableAllBuses();

    auto inputNode  = graph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));

    auto outputNode = graph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    audioInputNodeId  = inputNode->nodeID;
    audioOutputNodeId = outputNode->nodeID;

    // Default chain: input → output
    connectBlocks(audioInputNodeId, audioOutputNodeId);
}

StellarrProcessor::~StellarrProcessor() = default;

void StellarrProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    graph.setPlayConfigDetails(
        getTotalNumInputChannels(), getTotalNumOutputChannels(),
        sampleRate, samplesPerBlock);

    graph.prepareToPlay(sampleRate, samplesPerBlock);
}

void StellarrProcessor::releaseResources()
{
    graph.releaseResources();
}

void StellarrProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    graph.processBlock(buffer, midi);
}


// Graph management -----------------------------------------------------------

juce::AudioProcessorGraph::NodeID StellarrProcessor::addBlock(
    std::unique_ptr<stellarr::Block> block)
{
    auto node = graph.addNode(std::move(block));
    return node != nullptr ? node->nodeID
                           : juce::AudioProcessorGraph::NodeID{};
}

void StellarrProcessor::removeBlock(juce::AudioProcessorGraph::NodeID nodeId)
{
    // Prevent removal of the graph's built-in I/O nodes
    if (nodeId == audioInputNodeId || nodeId == audioOutputNodeId)
        return;

    graph.removeNode(nodeId);
}

bool StellarrProcessor::connectBlocks(
    juce::AudioProcessorGraph::NodeID source,
    juce::AudioProcessorGraph::NodeID dest,
    int numChannels)
{
    bool ok = true;

    for (int ch = 0; ch < numChannels; ++ch)
        ok &= graph.addConnection({{source, ch}, {dest, ch}});

    return ok;
}

void StellarrProcessor::disconnectBlocks(
    juce::AudioProcessorGraph::NodeID source,
    juce::AudioProcessorGraph::NodeID dest)
{
    for (auto& conn : graph.getConnections())
        if (conn.source.nodeID == source && conn.destination.nodeID == dest)
            graph.removeConnection(conn);
}

// AudioProcessor boilerplate -------------------------------------------------

juce::AudioProcessorEditor* StellarrProcessor::createEditor()
{
    return new StellarrEditor(*this);
}

bool StellarrProcessor::hasEditor() const { return true; }
const juce::String StellarrProcessor::getName() const { return JucePlugin_Name; }
bool StellarrProcessor::acceptsMidi() const { return false; }
bool StellarrProcessor::producesMidi() const { return false; }
double StellarrProcessor::getTailLengthSeconds() const { return 0.0; }
int StellarrProcessor::getNumPrograms() { return 1; }
int StellarrProcessor::getCurrentProgram() { return 0; }
void StellarrProcessor::setCurrentProgram(int) {}
const juce::String StellarrProcessor::getProgramName(int) { return {}; }
void StellarrProcessor::changeProgramName(int, const juce::String&) {}
void StellarrProcessor::getStateInformation(juce::MemoryBlock&) {}
void StellarrProcessor::setStateInformation(const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StellarrProcessor();
}
