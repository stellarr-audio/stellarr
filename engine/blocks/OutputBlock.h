#pragma once
#include "Block.h"
#include <atomic>

namespace stellarr
{

class OutputBlock final : public Block
{
public:
    OutputBlock() : Block("Output", 2, 2, false) {}

    BlockType getBlockType() const override { return BlockType::output; }

    void setTunerMute(bool mute) { tunerMuted.store(mute, std::memory_order_relaxed); }
    bool isTunerMuted() const { return tunerMuted.load(std::memory_order_relaxed); }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        if (tunerMuted.load(std::memory_order_relaxed))
            buffer.clear();
    }

private:
    std::atomic<bool> tunerMuted { false };
};

} // namespace stellarr
