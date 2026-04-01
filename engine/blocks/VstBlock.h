#pragma once
#include "Block.h"

namespace stellarr
{

// VST host block. Currently a pass-through placeholder.
// Step 2 adds AudioPluginInstance wrapping and plugin selection.
class VstBlock final : public Block
{
public:
    VstBlock() : Block("VST", 2, 2) {}

    BlockType getBlockType() const override { return BlockType::vst; }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
};

} // namespace stellarr
