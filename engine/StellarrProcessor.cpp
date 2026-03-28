#include "StellarrProcessor.h"
#include "StellarrEditor.h"

StellarrProcessor::StellarrProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

StellarrProcessor::~StellarrProcessor() = default;

void StellarrProcessor::prepareToPlay(double, int) {}
void StellarrProcessor::releaseResources() {}

void StellarrProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

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
