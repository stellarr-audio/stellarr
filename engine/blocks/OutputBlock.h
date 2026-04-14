#pragma once
#include "Block.h"
#include <atomic>
#include <cmath>
#include <limits>

namespace stellarr
{

class OutputBlock final : public Block
{
public:
    OutputBlock() : Block("Output", 2, 2, false)
    {
        setMeasureLoudness(true); // Output always measures
    }

    BlockType getBlockType() const override { return BlockType::output; }

    void setTunerMute(bool mute) { tunerMuted.store(mute, std::memory_order_relaxed); }
    bool isTunerMuted() const { return tunerMuted.load(std::memory_order_relaxed); }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        if (tunerMuted.load(std::memory_order_relaxed))
            buffer.clear();
    }

    // Target loudness in LUFS. NaN means no target set.
    void setTargetLufs(float lufs) { targetLufs.store(lufs, std::memory_order_relaxed); }
    float getTargetLufs() const { return targetLufs.load(std::memory_order_relaxed); }
    bool hasTargetLufs() const { return ! std::isnan(targetLufs.load(std::memory_order_relaxed)); }

    juce::var toJson() const override
    {
        auto v = Block::toJson();
        if (auto* obj = v.getDynamicObject())
        {
            const float t = targetLufs.load(std::memory_order_relaxed);
            if (! std::isnan(t)) obj->setProperty("targetLufs", static_cast<double>(t));
        }
        return v;
    }

    void fromJson(const juce::var& json) override
    {
        Block::fromJson(json);
        if (auto* obj = json.getDynamicObject())
        {
            if (obj->hasProperty("targetLufs"))
                setTargetLufs(static_cast<float>(static_cast<double>(obj->getProperty("targetLufs"))));
            else
                setTargetLufs(std::numeric_limits<float>::quiet_NaN());
        }
    }

private:
    std::atomic<bool> tunerMuted { false };
    std::atomic<float> targetLufs { std::numeric_limits<float>::quiet_NaN() };
};

} // namespace stellarr
