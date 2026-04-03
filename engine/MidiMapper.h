#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <functional>

class MidiMapper
{
public:
    enum class Target
    {
        presetChange,
        sceneSwitch,
        blockBypass,
        blockMix,
        blockBalance,
        blockLevel,
        tunerToggle,
    };

    struct Mapping
    {
        int channel = -1;       // 0-15 or -1 for any
        int ccNumber = -1;      // CC number, or -1 for Program Change
        Target target;
        juce::String blockId;   // for block-specific targets
        int targetIndex = -1;   // for scene/preset index (when using specific CC values)
    };

    // Process incoming MIDI — intercepts mapped events, removes them from buffer
    void processMidi(juce::MidiBuffer& midi);

    // Mapping management
    void addMapping(const Mapping& mapping);
    void removeMapping(int index);
    void clearAll();
    int getNumMappings() const { return static_cast<int>(mappings.size()); }
    const Mapping& getMapping(int index) const { return mappings[static_cast<size_t>(index)]; }
    const std::vector<Mapping>& getMappings() const { return mappings; }

    // MIDI Learn
    void startLearn(Target target, const juce::String& blockId = {});
    void cancelLearn();
    bool isLearning() const { return learning; }

    // Serialization
    juce::var toJson() const;
    void fromJson(const juce::var& json);

    // Split serialization: preset-level vs global mappings
    juce::var presetMappingsToJson() const;
    juce::var globalMappingsToJson() const;
    void loadPresetMappings(const juce::var& json);
    void loadGlobalMappings(const juce::var& json);

    static bool isGlobalTarget(Target t) { return t == Target::presetChange || t == Target::tunerToggle; }

    // Callbacks — set by the bridge
    std::function<void(int index)> onPresetChange;
    std::function<void(int index)> onSceneSwitch;
    std::function<void(const juce::String& blockId, bool state)> onBlockBypass;
    std::function<void(const juce::String& blockId, float value)> onBlockMix;
    std::function<void(const juce::String& blockId, float value)> onBlockBalance;
    std::function<void(const juce::String& blockId, float levelDb)> onBlockLevel;
    std::function<void(bool enabled)> onTunerToggle;
    std::function<void(int channel, int cc, int value)> onMidiActivity;
    std::function<void(int channel, int cc)> onLearnComplete;

    static juce::String targetToString(Target t);
    static Target targetFromString(const juce::String& s);

    // Monitor: ring buffer of recent MIDI events for UI display
    struct MonitorEvent
    {
        juce::String type;   // "CC", "Note On", "Note Off", "PC", "Other"
        int channel = 0;
        int data1 = 0;       // CC number, note number, or PC number
        int data2 = 0;       // CC value, velocity, or 0
    };

    static constexpr int monitorBufferSize = 64;
    std::array<MonitorEvent, monitorBufferSize> monitorBuffer {};
    std::atomic<int> monitorWritePos { 0 };
    std::atomic<int> monitorReadPos { 0 };
    std::atomic<bool> monitorEnabled { false };

    void pushMonitorEvent(const juce::MidiMessage& msg);

public:
    void setMonitorEnabled(bool enabled) { monitorEnabled.store(enabled, std::memory_order_relaxed); }
    bool isMonitorEnabled() const { return monitorEnabled.load(std::memory_order_relaxed); }

    // Drain events since last read (called from message thread timer)
    std::vector<MonitorEvent> drainMonitorEvents();

    // Inject a MIDI message into the next process buffer
    void injectMidi(const juce::MidiMessage& msg);
    void drainInjected(juce::MidiBuffer& dest);

private:
    std::vector<juce::MidiMessage> injectedMessages;
    juce::SpinLock injectLock;

    // Scale CC 0-127 to parameter ranges
    static float ccToMix(int value)     { return static_cast<float>(value) / 127.0f; }
    static float ccToBalance(int value) { return (static_cast<float>(value) / 63.5f) - 1.0f; }
    static float ccToLevelDb(int value) { return -60.0f + (static_cast<float>(value) / 127.0f) * 72.0f; }

    std::vector<Mapping> mappings;
    juce::SpinLock lock;

    // Learn state
    bool learning = false;
    Target learnTarget = Target::blockMix;
    juce::String learnBlockId;
};
