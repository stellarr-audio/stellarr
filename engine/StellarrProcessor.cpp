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

    auto midiInNode  = graph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
    auto midiOutNode = graph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode));

    midiInputNodeId  = midiInNode->nodeID;
    midiOutputNodeId = midiOutNode->nodeID;

    // Default chain: input → output (audio + MIDI)
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

    // Intercept Stellarr-mapped MIDI before graph processing
    midiMapper.processMidi(midi);

    auto start = juce::Time::getHighResolutionTicks();
    graph.processBlock(buffer, midi);
    auto end = juce::Time::getHighResolutionTicks();

    auto elapsedSecs = juce::Time::highResolutionTicksToSeconds(end - start);
    auto bufferDuration = static_cast<double>(buffer.getNumSamples()) / getSampleRate();
    cpuUsagePercent.store(elapsedSecs / bufferDuration * 100.0, std::memory_order_relaxed);

    // Peak output level across all channels (hold highest since last read)
    {
        float peak = 0.0f;
        if (buffer.getNumSamples() > 0)
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));

        float prev = outputPeakLevel.load(std::memory_order_relaxed);
        if (peak > prev)
            outputPeakLevel.store(peak, std::memory_order_relaxed);
    }
}


// Graph management -----------------------------------------------------------

juce::AudioProcessorGraph::NodeID StellarrProcessor::addBlock(
    std::unique_ptr<stellarr::Block> block, StellarrProcessor::UpdateKind update)
{
    auto node = graph.addNode(std::move(block), std::nullopt, update);
    return node != nullptr ? node->nodeID
                           : juce::AudioProcessorGraph::NodeID{};
}

void StellarrProcessor::removeBlock(juce::AudioProcessorGraph::NodeID nodeId,
                                    StellarrProcessor::UpdateKind update)
{
    // Prevent removal of the graph's built-in I/O nodes
    if (nodeId == audioInputNodeId || nodeId == audioOutputNodeId ||
        nodeId == midiInputNodeId || nodeId == midiOutputNodeId)
        return;

    graph.removeNode(nodeId, update);
}

bool StellarrProcessor::connectBlocks(
    juce::AudioProcessorGraph::NodeID source,
    juce::AudioProcessorGraph::NodeID dest,
    int numChannels, StellarrProcessor::UpdateKind update)
{
    bool ok = true;

    for (int ch = 0; ch < numChannels; ++ch)
        ok &= graph.addConnection({{source, ch}, {dest, ch}}, update);

    // Also connect MIDI channel
    graph.addConnection({
        {source, juce::AudioProcessorGraph::midiChannelIndex},
        {dest, juce::AudioProcessorGraph::midiChannelIndex}
    }, update);

    return ok;
}

void StellarrProcessor::disconnectBlocks(
    juce::AudioProcessorGraph::NodeID source,
    juce::AudioProcessorGraph::NodeID dest,
    StellarrProcessor::UpdateKind update)
{
    for (auto& conn : graph.getConnections())
        if (conn.source.nodeID == source && conn.destination.nodeID == dest)
            graph.removeConnection(conn, update);
}

// AudioProcessor boilerplate -------------------------------------------------

juce::AudioProcessorEditor* StellarrProcessor::createEditor()
{
    return new StellarrEditor(*this);
}

bool StellarrProcessor::hasEditor() const { return true; }
const juce::String StellarrProcessor::getName() const { return JucePlugin_Name; }
bool StellarrProcessor::acceptsMidi() const { return true; }
bool StellarrProcessor::producesMidi() const { return true; }
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
