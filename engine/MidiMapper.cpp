#include "MidiMapper.h"
#include "bridge/BridgeJson.h"
#include <algorithm>
#include <cmath>

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

float MidiMapper::normalisedCc(int value, int ccMin, int ccMax, Curve curve)
{
    if (ccMax <= ccMin) return 0.0f;

    const int clamped = juce::jlimit(ccMin, ccMax, value);
    const float t = static_cast<float>(clamped - ccMin) / static_cast<float>(ccMax - ccMin);

    switch (curve)
    {
        case Curve::Linear:
            return t;
        case Curve::Log:
            // Logarithmic — concentrates resolution at low end.
            return std::log10(1.0f + 9.0f * t);
        case Curve::Exp:
            // Exponential — concentrates resolution at high end.
            return (std::pow(10.0f, t) - 1.0f) / 9.0f;
        case Curve::Sigmoid:
        {
            // S-curve — smooth transition with steeper middle.
            const float k = 5.0f;
            const float top = std::tanh((t - 0.5f) * k);
            const float bot = std::tanh(0.5f * k);
            return 0.5f + 0.5f * top / bot;
        }
    }
    return t;
}

float MidiMapper::scaleToParam(float t, float paramMin, float paramMax,
                                float defaultMin, float defaultMax)
{
    const float mn = std::isnan(paramMin) ? defaultMin : paramMin;
    const float mx = std::isnan(paramMax) ? defaultMax : paramMax;
    return mn + t * (mx - mn);
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
            // Snapshot shaping into the event itself so a re-learn before the
            // message thread drains this event doesn't bind the new shaping
            // to the previous queued completion.
            evt.learnCcMin    = learnCcMin;
            evt.learnCcMax    = learnCcMax;
            evt.learnParamMin = learnParamMin;
            evt.learnParamMax = learnParamMax;
            evt.learnCurve    = learnCurve;
            evt.learnThreshold = learnThreshold;
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
                        // CC >= threshold engages the block (clears bypass). evt.value carries
                        // the "bypassed" bool; engaged means bypassed=false.
                        evt.value = (value >= m.threshold) ? 0 : 1;
                        pushOutbound(evt);
                        break;

                    case Target::blockMix:
                    {
                        evt.kind = OutboundEvent::Kind::blockMix;
                        const float t = normalisedCc(value, m.ccMin, m.ccMax, m.curve);
                        evt.floatValue = scaleToParam(t, m.paramMin, m.paramMax, 0.0f, 1.0f);
                        pushOutbound(evt);
                        break;
                    }

                    case Target::blockBalance:
                    {
                        evt.kind = OutboundEvent::Kind::blockBalance;
                        const float t = normalisedCc(value, m.ccMin, m.ccMax, m.curve);
                        evt.floatValue = scaleToParam(t, m.paramMin, m.paramMax, -1.0f, 1.0f);
                        pushOutbound(evt);
                        break;
                    }

                    case Target::blockLevel:
                    {
                        evt.kind = OutboundEvent::Kind::blockLevel;
                        const float t = normalisedCc(value, m.ccMin, m.ccMax, m.curve);
                        evt.floatValue = scaleToParam(t, m.paramMin, m.paramMax, -60.0f, 12.0f);
                        pushOutbound(evt);
                        break;
                    }

                    case Target::tunerToggle:
                        evt.kind = OutboundEvent::Kind::tunerToggle;
                        evt.value = (value >= m.threshold) ? 1 : 0;
                        pushOutbound(evt);
                        break;

                    case Target::blockState:
                        if (value >= m.threshold)
                        {
                            evt.kind = OutboundEvent::Kind::blockState;
                            evt.targetIndex = static_cast<int8_t>(m.targetIndex);
                            pushOutbound(evt);
                        }
                        else
                        {
                            // CC < threshold ignored. Still consumed so the message does not pass through.
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
                // Carry the dialog's shaping state from the snapshot baked
                // into the event when the audio thread queued it.
                m.ccMin    = evt.learnCcMin;
                m.ccMax    = evt.learnCcMax;
                m.paramMin = evt.learnParamMin;
                m.paramMax = evt.learnParamMax;
                m.curve    = evt.learnCurve;
                m.threshold = evt.learnThreshold;

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

void MidiMapper::startLearn(const LearnArgs& args)
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    learnTarget = args.target;
    learnBlockId = args.blockId;
    learnTargetIndex = args.targetIndex;
    learnCcMin = args.ccMin;
    learnCcMax = args.ccMax;
    learnParamMin = args.paramMin;
    learnParamMax = args.paramMax;
    learnCurve = args.curve;
    learnThreshold = args.threshold;
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

juce::String MidiMapper::curveToString(Curve c)
{
    switch (c)
    {
        case Curve::Linear:  return "linear";
        case Curve::Log:     return "log";
        case Curve::Exp:     return "exp";
        case Curve::Sigmoid: return "sigmoid";
    }
    return "linear";
}

MidiMapper::Curve MidiMapper::curveFromString(const juce::String& s)
{
    static const std::array<std::pair<const char*, Curve>, 4> map {{
        { "linear",  Curve::Linear  },
        { "log",     Curve::Log     },
        { "exp",     Curve::Exp     },
        { "sigmoid", Curve::Sigmoid },
    }};

    for (const auto& [name, curve] : map)
        if (s == name) return curve;

    return Curve::Linear;
}

static juce::var mappingToJson(const MidiMapper::Mapping& m)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("channel", m.channel);
    obj->setProperty("cc", m.ccNumber);
    obj->setProperty("target", MidiMapper::targetToString(m.target));
    if (m.blockId.isNotEmpty())     obj->setProperty("blockId", m.blockId);
    if (m.targetIndex >= 0)         obj->setProperty("targetIndex", m.targetIndex);
    if (m.ccMin != 0)               obj->setProperty("ccMin", m.ccMin);
    if (m.ccMax != 127)             obj->setProperty("ccMax", m.ccMax);
    if (! std::isnan(m.paramMin))   obj->setProperty("paramMin", static_cast<double>(m.paramMin));
    if (! std::isnan(m.paramMax))   obj->setProperty("paramMax", static_cast<double>(m.paramMax));
    if (m.curve != MidiMapper::Curve::Linear)
        obj->setProperty("curve", MidiMapper::curveToString(m.curve));
    if (m.threshold != 64)          obj->setProperty("threshold", m.threshold);
    return juce::var(obj);
}

static MidiMapper::Mapping parseMapping(const juce::DynamicObject& obj)
{
    using namespace stellarr::bridge::json;
    MidiMapper::Mapping m;
    m.channel     = static_cast<int>(obj.getProperty("channel"));
    m.ccNumber    = static_cast<int>(obj.getProperty("cc"));
    m.target      = MidiMapper::targetFromString(obj.getProperty("target").toString());
    m.blockId     = obj.getProperty("blockId").toString();
    m.targetIndex = getOptInt        (obj, "targetIndex", -1);
    m.ccMin       = getOptInt        (obj, "ccMin",        0);
    m.ccMax       = getOptInt        (obj, "ccMax",      127);
    m.paramMin    = getOptFloat      (obj, "paramMin", std::numeric_limits<float>::quiet_NaN());
    m.paramMax    = getOptFloat      (obj, "paramMax", std::numeric_limits<float>::quiet_NaN());
    m.curve       = MidiMapper::curveFromString(getOptString(obj, "curve", "linear"));
    m.threshold   = getOptIntClamped (obj, "threshold", 1, 127, 64);
    return m;
}

juce::var MidiMapper::toJson() const
{
    juce::Array<juce::var> arr;
    for (auto& m : mappings) arr.add(mappingToJson(m));
    return arr;
}

void MidiMapper::fromJson(const juce::var& json)
{
    juce::SpinLock::ScopedLockType scopedLock(mappingsLock);
    mappings.clear();
    if (auto* arr = json.getArray())
        for (auto& item : *arr)
            if (auto* obj = item.getDynamicObject())
                mappings.push_back(parseMapping(*obj));
}

// -- Split serialization (preset vs global) -----------------------------------

static juce::var filterMappingsToJson(const std::vector<MidiMapper::Mapping>& mappings, bool global)
{
    juce::Array<juce::var> arr;
    for (auto& m : mappings)
    {
        if (MidiMapper::isGlobalTarget(m.target) != global) continue;
        arr.add(mappingToJson(m));
    }
    return arr;
}

static std::vector<MidiMapper::Mapping> parseMappingsArray(const juce::var& json)
{
    std::vector<MidiMapper::Mapping> result;
    if (auto* arr = json.getArray())
        for (auto& item : *arr)
            if (auto* obj = item.getDynamicObject())
                result.push_back(parseMapping(*obj));
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
