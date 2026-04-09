#include "Block.h"

namespace stellarr
{

void Block::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    if (bypassed.load(std::memory_order_relaxed))
    {
        auto mode = getBypassMode();

        switch (mode)
        {
            case BypassMode::thru:
                return; // audio passes through unchanged

            case BypassMode::muteIn:
                buffer.clear(); // silence input, but let process() run (tails ring)
                process(buffer, midi);
                return;

            case BypassMode::muteOut:
                process(buffer, midi); // process runs (signal enters effect)
                buffer.clear();        // but output is silenced (tails cut)
                return;

            case BypassMode::mute:
                buffer.clear(); // total silence
                return;
        }
    }

    auto mixVal = mix.load(std::memory_order_relaxed);

    if (!hasMix || mixVal >= 1.0f)
    {
        process(buffer, midi);
    }
    else if (mixVal > 0.0f)
    {
        // Save dry signal
        if (dryBuffer.getNumChannels() != buffer.getNumChannels()
            || dryBuffer.getNumSamples() != buffer.getNumSamples())
        {
            dryBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
        }

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

        process(buffer, midi);

        float dryGain = 1.0f - mixVal;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                wet[i] = dry[i] * dryGain + wet[i] * mixVal;
        }
    }
    // mixVal <= 0.0f: fully dry, buffer untouched

    // Apply balance (stereo only, skip for mono or when centred)
    if (hasMix && buffer.getNumChannels() >= 2)
    {
        auto bal = balance.load(std::memory_order_relaxed);

        if (bal < -0.001f || bal > 0.001f)
        {
            float leftGain  = 1.0f - std::max(0.0f, bal);
            float rightGain = 1.0f + std::min(0.0f, bal);

            buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
            buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
        }
    }

    // Apply output level (all blocks, independent of mix)
    {
        auto lvl = level.load(std::memory_order_relaxed);
        if (lvl < 0.999f || lvl > 1.001f)
            buffer.applyGain(lvl);
    }

    // Update peak level for metering
    {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            peak = std::max(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));

        float prev = peakLevel.load(std::memory_order_relaxed);
        if (peak > prev)
            peakLevel.store(peak, std::memory_order_relaxed);
    }
}

void Block::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (hasMix)
        dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock, false, false, true);

    prepareBlock(sampleRate, samplesPerBlock);
}

juce::var Block::toJson() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id", blockId.toString());
    obj->setProperty("type", blockTypeToString(getBlockType()));
    obj->setProperty("name", blockName);
    if (displayName.isNotEmpty())
        obj->setProperty("displayName", displayName);
    if (blockColor.isNotEmpty())
        obj->setProperty("blockColor", blockColor);

    obj->setProperty("level", static_cast<double>(getLevelDb()));

    if (hasMix)
    {
        obj->setProperty("mix", static_cast<double>(getMix()));
        obj->setProperty("balance", static_cast<double>(getBalance()));
        obj->setProperty("bypassed", isBypassed());
        obj->setProperty("bypassMode", bypassModeToString(getBypassMode()));
    }

    return juce::var(obj);
}

void Block::fromJson(const juce::var& json)
{
    if (auto* obj = json.getDynamicObject())
    {
        auto idStr = obj->getProperty("id").toString();
        if (idStr.isNotEmpty())
            blockId = juce::Uuid(idStr);

        if (obj->hasProperty("displayName"))
            displayName = obj->getProperty("displayName").toString();
        if (obj->hasProperty("blockColor"))
            blockColor = obj->getProperty("blockColor").toString();

        if (obj->hasProperty("level"))
            setLevelDb(static_cast<float>(obj->getProperty("level")));

        if (hasMix && obj->hasProperty("mix"))
            setMix(static_cast<float>(obj->getProperty("mix")));

        if (hasMix && obj->hasProperty("balance"))
            setBalance(static_cast<float>(obj->getProperty("balance")));

        if (hasMix && obj->hasProperty("bypassed"))
            setBypassed(static_cast<bool>(obj->getProperty("bypassed")));

        if (hasMix && obj->hasProperty("bypassMode"))
            setBypassMode(bypassModeFromString(obj->getProperty("bypassMode").toString()));
    }
}

} // namespace stellarr
