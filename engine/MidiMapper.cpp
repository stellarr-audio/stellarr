#include "MidiMapper.h"

void MidiMapper::processMidi(juce::MidiBuffer& midi)
{
    // Inject any queued test messages
    drainInjected(midi);

    juce::SpinLock::ScopedLockType scopedLock(lock);

    juce::MidiBuffer filtered;

    for (const auto metadata : midi)
    {
        auto msg = metadata.getMessage();
        bool consumed = false;

        // Push to monitor ring buffer
        if (monitorEnabled.load(std::memory_order_relaxed))
            pushMonitorEvent(msg);

        // MIDI Learn — first CC received creates the mapping
        if (learning && msg.isController())
        {
            Mapping m;
            m.channel = msg.getChannel() - 1;
            m.ccNumber = msg.getControllerNumber();
            m.target = learnTarget;
            m.blockId = learnBlockId;
            mappings.push_back(m);

            learning = false;

            if (onLearnComplete)
                onLearnComplete(m.channel, m.ccNumber);

            consumed = true;
        }

        // Activity callback (for UI monitoring)
        if (msg.isController() && onMidiActivity)
            onMidiActivity(msg.getChannel() - 1, msg.getControllerNumber(), msg.getControllerValue());

        // Check CC mappings
        if (!consumed && msg.isController())
        {
            int ch = msg.getChannel() - 1;
            int cc = msg.getControllerNumber();
            int value = msg.getControllerValue();

            for (auto& m : mappings)
            {
                if (m.ccNumber != cc) continue;
                if (m.channel >= 0 && m.channel != ch) continue;

                consumed = true;

                switch (m.target)
                {
                    case Target::sceneSwitch:
                        if (onSceneSwitch)
                            onSceneSwitch(value); // CC value = scene index
                        break;

                    case Target::blockBypass:
                        if (onBlockBypass)
                            onBlockBypass(m.blockId, value >= 64);
                        break;

                    case Target::blockMix:
                        if (onBlockMix)
                            onBlockMix(m.blockId, ccToMix(value));
                        break;

                    case Target::blockBalance:
                        if (onBlockBalance)
                            onBlockBalance(m.blockId, ccToBalance(value));
                        break;

                    case Target::blockLevel:
                        if (onBlockLevel)
                            onBlockLevel(m.blockId, ccToLevelDb(value));
                        break;

                    case Target::tunerToggle:
                        if (onTunerToggle)
                            onTunerToggle(value >= 64);
                        break;

                    case Target::presetChange:
                        // Handled by Program Change below
                        consumed = false;
                        break;
                }
            }
        }

        // Check Program Change mappings
        if (!consumed && msg.isProgramChange())
        {
            int ch = msg.getChannel() - 1;
            int pc = msg.getProgramChangeNumber();

            for (auto& m : mappings)
            {
                if (m.target != Target::presetChange) continue;
                if (m.ccNumber != -1) continue; // -1 means "respond to PC"
                if (m.channel >= 0 && m.channel != ch) continue;

                consumed = true;
                if (onPresetChange)
                    onPresetChange(pc);
                break;
            }
        }

        if (!consumed)
            filtered.addEvent(msg, metadata.samplePosition);
    }

    midi.swapWith(filtered);
}

// -- Mapping management -------------------------------------------------------

void MidiMapper::addMapping(const Mapping& mapping)
{
    juce::SpinLock::ScopedLockType scopedLock(lock);
    mappings.push_back(mapping);
}

void MidiMapper::removeMapping(int index)
{
    juce::SpinLock::ScopedLockType scopedLock(lock);
    if (index >= 0 && index < static_cast<int>(mappings.size()))
        mappings.erase(mappings.begin() + index);
}

void MidiMapper::clearAll()
{
    juce::SpinLock::ScopedLockType scopedLock(lock);
    mappings.clear();
}

// -- MIDI Learn ---------------------------------------------------------------

void MidiMapper::startLearn(Target target, const juce::String& blockId)
{
    juce::SpinLock::ScopedLockType scopedLock(lock);
    learning = true;
    learnTarget = target;
    learnBlockId = blockId;
}

void MidiMapper::cancelLearn()
{
    juce::SpinLock::ScopedLockType scopedLock(lock);
    learning = false;
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
    }
    return "unknown";
}

MidiMapper::Target MidiMapper::targetFromString(const juce::String& s)
{
    if (s == "presetChange") return Target::presetChange;
    if (s == "sceneSwitch")  return Target::sceneSwitch;
    if (s == "blockBypass")  return Target::blockBypass;
    if (s == "blockMix")     return Target::blockMix;
    if (s == "blockBalance") return Target::blockBalance;
    if (s == "blockLevel")   return Target::blockLevel;
    if (s == "tunerToggle")  return Target::tunerToggle;
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
        arr.add(juce::var(obj));
    }
    return arr;
}

void MidiMapper::fromJson(const juce::var& json)
{
    juce::SpinLock::ScopedLockType scopedLock(lock);
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
    juce::SpinLock::ScopedLockType scopedLock(lock);

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
    juce::SpinLock::ScopedLockType scopedLock(lock);

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
    juce::SpinLock::ScopedLockType scopedLock(injectLock);
    injectedMessages.push_back(msg);
}

void MidiMapper::drainInjected(juce::MidiBuffer& dest)
{
    juce::SpinLock::ScopedLockType scopedLock(injectLock);
    for (auto& msg : injectedMessages)
        dest.addEvent(msg, 0);
    injectedMessages.clear();
}
