#pragma once
#include "Block.h"

namespace stellarr
{

class OutputBlock final : public Block
{
public:
    OutputBlock() : Block("Output", 2, 2, false) {}

    BlockType getBlockType() const override { return BlockType::output; }

    void process(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
};

} // namespace stellarr
