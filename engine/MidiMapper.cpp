#include "MidiMapper.h"
#include <algorithm>

MidiMapper::MidiMapper()
{
    scratchBuffer.ensureSize(2048);
}

void MidiMapper::copyBlockId(std::array<char, 40>& dst, const juce::String& src)
{
    dst.fill(0);
    if (src.isEmpty()) return;

    const auto* utf8 = src.toRawUTF8();
    if (utf8 == nullptr) return;

    const size_t maxBytes = dst.size() - 1;
    size_t i = 0;
    while (i < maxBytes && utf8[i] != '\0')
    {
        dst[i] = utf8[i];
        ++i;
    }
}

bool MidiMapper::pushOutbound(const OutboundEvent& evt)
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    outboundFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 + size2 == 0)
    {
        outboundOverflowCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (size1 > 0)
        outboundRing[static_cast<size_t>(start1)] = evt;
    else
        outboundRing[static_cast<size_t>(start2)] = evt;

    outboundFifo.finishedWrite(1);
    return true;
}

void MidiMapper::processMidi(juce::MidiBuffer& midi)
{
    // Drain queued inject messages (lock-free SPSC).
    drainInjected(midi);

    // Try to acquire the mappings lock without blocking. If the message thread
    // is mid-mutation, skip mapping evaluation for this block — events pass
    // through unfiltered. Mappings will apply on the next block.
    juce::SpinLock::ScopedTryLockType mappingsScopedLock(mappingsLock);
    const bool haveMappings = mappingsScopedLock.isLocked();

    scratchBuffer.clear();

    for (const auto metadata : midi)
    {
        auto msg = metadata.getMessage();
        bool consumed = false;

        if (monitorEnabled.load(std::memory_order_relaxed))
            pushMonitorEvent(msg);

        // MIDI Learn — first CC during learning enqueues a LearnComplete event
        // for the message thread to apply. The audio thread does not mutate
        // the mappings vector itself. learnTarget / learnBlockId are written
        // by startLearn() under mappingsLock, so we may only read them when
        // we hold the lock here.
        if (haveMappings && learning.load(std::memory_order_acquire)
                         && msg.isController())
        {
            OutboundEvent evt;
            evt.kind = OutboundEvent::Kind::learnComplete;
            evt.channel = msg.getChannel() - 1;
            evt.cc = msg.getControllerNumber();
            evt.learnTarget = learnTarget;
            evt.targetIndex = static_cast<int8_t>(learnTargetIndex);
            copyBlockId(evt.blockId, learnBlockId);

            // Only latch off and consume the CC if the message thread will
            // actually receive the learnComplete event. If the outbound fifo
            // is full (message thread temporarily stalled), keep learning
            // active so the next CC retries instead of silently dropping
            // the learn operation.
            if (pushOutbound(evt))
            {
                learning.store(false, std::memory_order_release);
                consumed = true;
            }
        }

        // Activity event for the optional onMidiActivity callback. Gated on a
        // runtime flag so production (where the callback is unset) does not
        // burn fifo headroom on events nothing consumes.
        if (msg.isController() && activityEventsEnabled.load(std::memory_order_relaxed))
        {
            OutboundEvent evt;
            evt.kind = OutboundEvent::Kind::midiActivity;
            evt.channel = msg.getChannel() - 1;
            evt.cc = msg.getControllerNumber();
            evt.value = msg.getControllerValue();
            pushOutbound(evt);
        }

        // CC mapping evaluation — only when we hold the mappings lock.
        if (haveMappings && !consumed && msg.isController())
        {
            const int ch = msg.getChannel() - 1;
            const int cc = msg.getControllerNumber();
            const int value = msg.getControllerValue();

            for (auto& m : mappings)
            {
                if (m.ccNumber != cc) continue;
                if (m.channel >= 0 && m.channel != ch) continue;

                consumed = true;

                OutboundEvent evt;
                evt.channel = ch;
                evt.cc = cc;
                evt.value = value;
                copyBlockId(evt.blockId, m.blockId);

                switch (m.target)
                {
                    case Target::sceneSwitch:
                        evt.kind = OutboundEvent::Kind::sceneSwitch;
                        evt.value = value; // CC value = scene index
                        pushOutbound(evt);
                        break;

                    case Target::blockBypass:
                        evt.kind = OutboundEvent::Kind::blockBypass;
                        evt.value = (value >= 64) ? 1 : 0;
                        pushOutbound(evt);
                        break;

                    case Target::blockMix:
                        evt.kind = OutboundEvent::Kind::blockMix;
                        evt.floatValue = ccToMix(value);
                        pushOutbound(evt);
                        break;

                    case Target::blockBalance:
                        evt.kind = OutboundEvent::Kind::blockBalance;
                        evt.floatValue = ccToBalance(value);
                        pushOutbound(evt);
                        break;

                    case Target::blockLevel:
                        evt.kind = OutboundEvent::Kind::blockLevel;
                        evt.floatValue = ccToLevelDb(value);
                        pushOutbound(evt);
                        break;

                    case Target::tunerToggle:
                        evt.kind = OutboundEvent::Kind::tunerToggle;
                        evt.value = (value >= 64) ? 1 : 0;
                        pushOutbound(evt);
                        break;

                    case Target::blockState:
                        if (value >= 64)
                        {
                            evt.kind = OutboundEvent::Kind::blockState;
                            evt.targetIndex = static_cast<int8_t>(m.targetIndex);
                            pushOutbound(evt);
                        }
                        else
                        {
                            // CC < 64 ignored. Still consumed so the message does not pass through.
                        }
                        break;

                    case Target::presetChange:
                        // Handled via Program Change below; CC is not the
                        // preset-change trigger.
                        consumed = false;
                        break;
                }
            }
        }

        // Program Change mapping evaluation
        if (haveMappings && !consumed && msg.isProgramChange())
        {
            const int ch = msg.getChannel() - 1;
            const int pc = msg.getProgramChangeNumber();

            for (auto& m : mappings)
            {
                if (m.target != Target::presetChange) continue;
                if (m.ccNumber != -1) continue; // -1 means "respond to PC"
                if (m.channel >= 0 && m.channel != ch) continue;

                OutboundEvent evt;
                evt.kind = OutboundEvent::Kind::presetChange;
                evt.channel = ch;
                evt.value = pc;
                pushOutbound(evt);

                consumed = true;
                break;
            }
        }

        if (!consumed)
            scratchBuffer.addEvent(msg, metadata.samplePosition);
    }

    midi.swapWith(scratchBuffer);
}

int MidiMapper::drainOutboundEvents()
{
    int dispatched = 0;

    while (outboundFifo.getNumReady() > 0)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        outboundFifo.prepareToRead(1, start1, size1, start2, size2);
        if (size1 + size2 == 0) break;

        const auto& evt = (size1 > 0)
            ? outboundRing[static_cast<size_t>(start1)]
            : outboundRing[static_cast<size_t>(start2)];

        const juce::String blockId(evt.blockId.data());

        switch (evt.kind)
        {
            case OutboundEvent::Kind::midiActivity:
                if (onMidiActivity)
                    onMidiActivity(evt.channel, evt.cc, evt.value);
                break;

            case OutboundEvent::Kind::learnComplete:
            {
                Mapping m;
                m.channel = evt.channel;
                m.ccNumber = evt.cc;
                m.target = evt.learnTarget;
                m.blockId = blockId;
                m.targetIndex = static_cast<int>(evt.targetIndex);
                addMapping(m);

                if (onLearnComplete)
                    onLearnComplete(evt.channel, evt.cc);
                break;
            }

            case OutboundEvent::Kind::presetChange:
                if (onPresetChange)
                    onPresetChange(evt.value);
                break;

            case OutboundEvent::Kind::sceneSwitch:
                if (onSceneSwitch)
                    onSceneSwitch(evt.value);
                break;

            case OutboundEvent::Kind::blockBypass:
                if (onBlockBypass)
                    onBlockBypass(blockId, evt.value != 0);
                break;

            case OutboundEvent::Kind::blockMix:
                if (onBlockMix)
                    onBlockMix(blockId, evt.floatValue);
                break;

            case OutboundEvent::Kind::blockBalance:
                if (onBlockBalance)
                    onBlockBalance(blockId, evt.floatValue);
                break;

            case OutboundEvent::Kind::blockLevel:
                if (onBlockLevel)
                    onBlockLevel(blockId, evt.floatValue);
                break;

            case OutboundEvent::Kind::tunerToggle:
                if (onTunerToggle)
                    onTunerToggle(evt.value != 0);
                break;

            case OutboundEvent::Kind::blockState:
                if (onBlockState)
                    onBlockState(blockId, static_cast<int>(evt.targetIndex));
                break;
        }

        outboundFifo.finishedRead(1);
        ++dispatched;
    }

    return dispatched;
}

// -- Mapping management -------------------------------------------------------

void MidiMapper::addMapping(const Mapping& mapping)
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    mappings.push_back(mapping);
}

void MidiMapper::removeMapping(int index)
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    if (index >= 0 && index < static_cast<int>(mappings.size()))
        mappings.erase(mappings.begin() + index);
}

void MidiMapper::clearAll()
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    mappings.clear();
}

void MidiMapper::removeMappingsForBlock(const juce::String& blockId)
{
    if (blockId.isEmpty()) return;
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    mappings.erase(std::remove_if(mappings.begin(), mappings.end(),
        [&](const Mapping& m) { return m.blockId == blockId; }), mappings.end());
}

void MidiMapper::removeMappingsForBlockState(const juce::String& blockId, int deletedIndex)
{
    if (blockId.isEmpty()) return;
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);

    mappings.erase(std::remove_if(mappings.begin(), mappings.end(),
        [&](const Mapping& m) {
            return m.target == Target::blockState
                && m.blockId == blockId
                && m.targetIndex == deletedIndex;
        }), mappings.end());

    for (auto& m : mappings)
    {
        if (m.target == Target::blockState
            && m.blockId == blockId
            && m.targetIndex > deletedIndex)
        {
            --m.targetIndex;
        }
    }
}

// -- MIDI Learn ---------------------------------------------------------------

void MidiMapper::startLearn(Target target, const juce::String& blockId, int targetIndex)
{
    // Serialise with the audio-thread try-lock in processMidi(): the audio
    // thread only inspects learnTarget / learnBlockId while holding
    // mappingsLock, so writes to those fields are also taken under the lock.
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    learnTarget = target;
    learnBlockId = blockId;
    learnTargetIndex = targetIndex;
    learning.store(true, std::memory_order_release);
}

void MidiMapper::cancelLearn()
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    learning.store(false, std::memory_order_release);
}

// -- Serialization ------------------------------------------------------------

juce::String MidiMapper::targetToString(Target t)
{
    switch (t)
    {
        case Target::presetChange: return "presetChange";
        case Target::sceneSwitch:  return "sceneSwitch";
        case Target::blockBypass:  return "blockBypass";
        case Target::blockMix:     return "blockMix";
        case Target::blockBalance: return "blockBalance";
        case Target::blockLevel:   return "blockLevel";
        case Target::tunerToggle:  return "tunerToggle";
        case Target::blockState:   return "blockState";
    }
    return "unknown";
}

MidiMapper::Target MidiMapper::targetFromString(const juce::String& s)
{
    static const std::array<std::pair<const char*, Target>, 8> map {{
        { "presetChange", Target::presetChange },
        { "sceneSwitch",  Target::sceneSwitch  },
        { "blockBypass",  Target::blockBypass  },
        { "blockMix",     Target::blockMix     },
        { "blockBalance", Target::blockBalance },
        { "blockLevel",   Target::blockLevel   },
        { "tunerToggle",  Target::tunerToggle  },
        { "blockState",   Target::blockState   },
    }};

    for (const auto& [name, target] : map)
        if (s == name) return target;

    return Target::blockMix;
}

juce::var MidiMapper::toJson() const
{
    juce::Array<juce::var> arr;
    for (auto& m : mappings)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("channel", m.channel);
        obj->setProperty("cc", m.ccNumber);
        obj->setProperty("target", targetToString(m.target));
        if (m.blockId.isNotEmpty())
            obj->setProperty("blockId", m.blockId);
        if (m.targetIndex >= 0)
            obj->setProperty("targetIndex", m.targetIndex);
        arr.add(juce::var(obj));
    }
    return arr;
}

void MidiMapper::fromJson(const juce::var& json)
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    mappings.clear();

    if (auto* arr = json.getArray())
    {
        for (auto& item : *arr)
        {
            if (auto* obj = item.getDynamicObject())
            {
                Mapping m;
                m.channel = static_cast<int>(obj->getProperty("channel"));
                m.ccNumber = static_cast<int>(obj->getProperty("cc"));
                m.target = targetFromString(obj->getProperty("target").toString());
                m.blockId = obj->getProperty("blockId").toString();
                auto tiVar = obj->getProperty("targetIndex");
                m.targetIndex = tiVar.isVoid() ? -1 : static_cast<int>(tiVar);
                mappings.push_back(m);
            }
        }
    }
}

// -- Split serialization (preset vs global) -----------------------------------

static juce::var filterMappingsToJson(const std::vector<MidiMapper::Mapping>& mappings, bool global)
{
    juce::Array<juce::var> arr;
    for (auto& m : mappings)
    {
        if (MidiMapper::isGlobalTarget(m.target) != global) continue;
        auto* obj = new juce::DynamicObject();
        obj->setProperty("channel", m.channel);
        obj->setProperty("cc", m.ccNumber);
        obj->setProperty("target", MidiMapper::targetToString(m.target));
        if (m.blockId.isNotEmpty())
            obj->setProperty("blockId", m.blockId);
        if (m.targetIndex >= 0)
            obj->setProperty("targetIndex", m.targetIndex);
        arr.add(juce::var(obj));
    }
    return arr;
}

static std::vector<MidiMapper::Mapping> parseMappingsArray(const juce::var& json)
{
    std::vector<MidiMapper::Mapping> result;
    if (auto* arr = json.getArray())
    {
        for (auto& item : *arr)
        {
            if (auto* obj = item.getDynamicObject())
            {
                MidiMapper::Mapping m;
                m.channel = static_cast<int>(obj->getProperty("channel"));
                m.ccNumber = static_cast<int>(obj->getProperty("cc"));
                m.target = MidiMapper::targetFromString(obj->getProperty("target").toString());
                m.blockId = obj->getProperty("blockId").toString();
                auto tiVar = obj->getProperty("targetIndex");
                m.targetIndex = tiVar.isVoid() ? -1 : static_cast<int>(tiVar);
                result.push_back(m);
            }
        }
    }
    return result;
}

juce::var MidiMapper::presetMappingsToJson() const
{
    return filterMappingsToJson(mappings, false);
}

juce::var MidiMapper::globalMappingsToJson() const
{
    return filterMappingsToJson(mappings, true);
}

void MidiMapper::loadPresetMappings(const juce::var& json)
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);

    // Keep global mappings, replace preset mappings
    std::vector<Mapping> globals;
    for (auto& m : mappings)
        if (isGlobalTarget(m.target))
            globals.push_back(m);

    mappings = globals;
    for (auto& m : parseMappingsArray(json))
        mappings.push_back(m);
}

void MidiMapper::loadGlobalMappings(const juce::var& json)
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);

    // Keep preset mappings, replace global mappings
    std::vector<Mapping> presets;
    for (auto& m : mappings)
        if (!isGlobalTarget(m.target))
            presets.push_back(m);

    mappings = presets;
    for (auto& m : parseMappingsArray(json))
        mappings.push_back(m);
}

// -- Monitor ------------------------------------------------------------------

void MidiMapper::pushMonitorEvent(const juce::MidiMessage& msg)
{
    MonitorEvent evt;
    evt.channel = msg.getChannel() - 1;

    if (msg.isController())
    {
        evt.type = "CC";
        evt.data1 = msg.getControllerNumber();
        evt.data2 = msg.getControllerValue();
    }
    else if (msg.isNoteOn())
    {
        evt.type = "Note On";
        evt.data1 = msg.getNoteNumber();
        evt.data2 = msg.getVelocity();
    }
    else if (msg.isNoteOff())
    {
        evt.type = "Note Off";
        evt.data1 = msg.getNoteNumber();
        evt.data2 = 0;
    }
    else if (msg.isProgramChange())
    {
        evt.type = "PC";
        evt.data1 = msg.getProgramChangeNumber();
        evt.data2 = 0;
    }
    else
    {
        evt.type = "Other";
        evt.data1 = msg.getRawData()[0];
        evt.data2 = msg.getRawDataSize() > 1 ? msg.getRawData()[1] : 0;
    }

    int pos = monitorWritePos.load(std::memory_order_relaxed);
    monitorBuffer[static_cast<size_t>(pos)] = evt;
    monitorWritePos.store((pos + 1) % monitorBufferSize, std::memory_order_release);
}

std::vector<MidiMapper::MonitorEvent> MidiMapper::drainMonitorEvents()
{
    std::vector<MonitorEvent> events;
    int read = monitorReadPos.load(std::memory_order_relaxed);
    int write = monitorWritePos.load(std::memory_order_acquire);

    while (read != write)
    {
        events.push_back(monitorBuffer[static_cast<size_t>(read)]);
        read = (read + 1) % monitorBufferSize;
    }

    monitorReadPos.store(read, std::memory_order_relaxed);
    return events;
}

// -- Inject -------------------------------------------------------------------

void MidiMapper::injectMidi(const juce::MidiMessage& msg)
{
    const int rawSize = msg.getRawDataSize();
    if (rawSize <= 0 || rawSize > 3)
        return;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    injectFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 + size2 == 0)
        return; // ring full, drop

    const int idx = (size1 > 0) ? start1 : start2;
    auto& slot = injectRing[static_cast<size_t>(idx)];
    slot.byteCount = rawSize;
    const auto* raw = msg.getRawData();
    for (int i = 0; i < rawSize; ++i)
        slot.bytes[i] = raw[i];
    for (int i = rawSize; i < 3; ++i)
        slot.bytes[i] = 0;

    injectFifo.finishedWrite(1);
}

void MidiMapper::drainInjected(juce::MidiBuffer& dest)
{
    while (injectFifo.getNumReady() > 0)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        injectFifo.prepareToRead(1, start1, size1, start2, size2);
        if (size1 + size2 == 0) break;

        const int idx = (size1 > 0) ? start1 : start2;
        const auto& slot = injectRing[static_cast<size_t>(idx)];

        if (slot.byteCount == 1)
            dest.addEvent(juce::MidiMessage(slot.bytes[0]), 0);
        else if (slot.byteCount == 2)
            dest.addEvent(juce::MidiMessage(slot.bytes[0], slot.bytes[1]), 0);
        else if (slot.byteCount == 3)
            dest.addEvent(juce::MidiMessage(slot.bytes[0], slot.bytes[1], slot.bytes[2]), 0);

        injectFifo.finishedRead(1);
    }
}
