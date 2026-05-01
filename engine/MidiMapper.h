#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <atomic>
#include <functional>
#include <limits>

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
        blockState,
    };

    enum class Curve : uint8_t { Linear, Log, Exp, Sigmoid };

    struct Mapping
    {
        int channel = -1;       // 0-15 or -1 for any
        int ccNumber = -1;      // CC number, or -1 for Program Change
        Target target;
        juce::String blockId;   // for block-specific targets
        int targetIndex = -1;   // scene/preset/state index; -1 when unused

        // Continuous-target shaping: maps CC range [ccMin, ccMax] to parameter
        // range [paramMin, paramMax] via the chosen curve. NaN paramMin/paramMax
        // is the "use target default" sentinel (engine substitutes target-specific
        // defaults at scaling time).
        int   ccMin    = 0;
        int   ccMax    = 127;
        float paramMin = std::numeric_limits<float>::quiet_NaN();
        float paramMax = std::numeric_limits<float>::quiet_NaN();
        Curve curve    = Curve::Linear;
    };

    struct LearnArgs
    {
        Target target;
        juce::String blockId;
        int targetIndex = -1;

        int   ccMin    = 0;
        int   ccMax    = 127;
        float paramMin = std::numeric_limits<float>::quiet_NaN();
        float paramMax = std::numeric_limits<float>::quiet_NaN();
        Curve curve    = Curve::Linear;
    };

    MidiMapper();

    // Process incoming MIDI — intercepts mapped events, removes them from buffer.
    // Audio-thread safe: mappings are read under a try-lock (skips this block if
    // the message thread is mid-mutation). All UI-side dispatch happens via the
    // outbound SPSC queue drained from the message thread.
    void processMidi(juce::MidiBuffer& midi);

    // Mapping management
    void addMapping(const Mapping& mapping);
    void removeMapping(int index);
    void clearAll();
    int getNumMappings() const { return static_cast<int>(mappings.size()); }
    const Mapping& getMapping(int index) const { return mappings[static_cast<size_t>(index)]; }
    const std::vector<Mapping>& getMappings() const { return mappings; }

    // Remove all mappings whose blockId matches. Used when a block is removed
    // from the graph or when a preset is replaced. Audio-thread safe via the
    // mappings spinlock.
    void removeMappingsForBlock(const juce::String& blockId);

    // Remove mappings for a specific (blockId, stateIndex) pair, and shift
    // targetIndex down by one for any blockState mapping where targetIndex is
    // strictly greater than the deleted index. Mirrors PluginBlock::deleteState
    // semantics. Audio-thread safe via the mappings spinlock.
    void removeMappingsForBlockState(const juce::String& blockId, int deletedIndex);

    // MIDI Learn
    void startLearn(const LearnArgs& args);
    void cancelLearn();
    bool isLearning() const { return learning.load(std::memory_order_acquire); }

    // Serialization
    juce::var toJson() const;
    void fromJson(const juce::var& json);

    // Split serialization: preset-level vs global mappings
    juce::var presetMappingsToJson() const;
    juce::var globalMappingsToJson() const;
    void loadPresetMappings(const juce::var& json);
    void loadGlobalMappings(const juce::var& json);

    static bool isGlobalTarget(Target t) { return t == Target::presetChange || t == Target::tunerToggle; }

    // Callbacks — set by the bridge. Called from the message thread after
    // drainOutboundEvents(); not from the audio thread.
    std::function<void(int index)> onPresetChange;
    std::function<void(int index)> onSceneSwitch;
    std::function<void(const juce::String& blockId, bool state)> onBlockBypass;
    std::function<void(const juce::String& blockId, float value)> onBlockMix;
    std::function<void(const juce::String& blockId, float value)> onBlockBalance;
    std::function<void(const juce::String& blockId, float levelDb)> onBlockLevel;
    std::function<void(bool enabled)> onTunerToggle;
    std::function<void(const juce::String& blockId, int stateIndex)> onBlockState;
    std::function<void(int channel, int cc, int value)> onMidiActivity;
    std::function<void(int channel, int cc)> onLearnComplete;

    // Drain outbound events on the message thread and dispatch to the callbacks
    // above. Returns the number of events dispatched. Should be called on a
    // periodic timer (~16 ms in production).
    int drainOutboundEvents();

    static juce::String targetToString(Target t);
    static Target targetFromString(const juce::String& s);
    static juce::String curveToString(Curve c);
    static Curve        curveFromString(const juce::String& s);

    // Test-only accessors — wrap the private static scaling helpers so
    // engine/test/ can exercise them directly.
    static float normalisedCcForTesting(int value, int ccMin, int ccMax, Curve curve)
    {
        return normalisedCc(value, ccMin, ccMax, curve);
    }
    static float scaleToParamForTesting(float t, float paramMin, float paramMax,
                                        float defaultMin, float defaultMax)
    {
        return scaleToParam(t, paramMin, paramMax, defaultMin, defaultMax);
    }

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
    std::atomic<bool> activityEventsEnabled { false };

    void pushMonitorEvent(const juce::MidiMessage& msg);

    void setMonitorEnabled(bool enabled) { monitorEnabled.store(enabled, std::memory_order_relaxed); }
    bool isMonitorEnabled() const { return monitorEnabled.load(std::memory_order_relaxed); }

    // Activity events feed the optional onMidiActivity callback. Disabled by
    // default so the audio thread does not spend fifo headroom on events
    // nothing consumes; callers wiring the callback should call this with true.
    void setActivityEventsEnabled(bool enabled) { activityEventsEnabled.store(enabled, std::memory_order_relaxed); }
    bool areActivityEventsEnabled() const { return activityEventsEnabled.load(std::memory_order_relaxed); }

    // Drain MIDI monitor events for UI display (called from message-thread timer)
    std::vector<MonitorEvent> drainMonitorEvents();

    // Inject a MIDI message into the next process buffer. UI side; lock-free
    // SPSC into a fixed-capacity ring of short MIDI messages.
    void injectMidi(const juce::MidiMessage& msg);
    void drainInjected(juce::MidiBuffer& dest);

    // Outbound overflow counter — increments on the audio thread when the
    // outbound fifo is full and an event is dropped. Read on message thread
    // for diagnostics.
    int getOutboundOverflowCount() const { return outboundOverflowCount.load(std::memory_order_relaxed); }

private:
    // Outbound event from audio thread to message thread
    struct OutboundEvent
    {
        enum class Kind : uint8_t
        {
            midiActivity,
            learnComplete,
            presetChange,
            sceneSwitch,
            blockBypass,
            blockMix,
            blockBalance,
            blockLevel,
            tunerToggle,
            blockState,
        };

        Kind kind = Kind::midiActivity;
        int channel = 0;
        int cc = 0;
        int value = 0;
        float floatValue = 0.0f;
        Target learnTarget = Target::blockMix;
        int8_t targetIndex = -1;          // stateIndex for blockState events and learn-complete payloads; -1 when unused
        std::array<char, 40> blockId {}; // null-terminated; UUID = 36 chars
    };

    static constexpr int outboundCapacity = 256;
    juce::AbstractFifo outboundFifo { outboundCapacity };
    std::array<OutboundEvent, outboundCapacity> outboundRing {};
    std::atomic<int> outboundOverflowCount { 0 };

    // Audio-thread helper: enqueue an outbound event. Returns false if the fifo
    // is full (event dropped, overflow counter incremented).
    bool pushOutbound(const OutboundEvent& evt);

    // Inject ring (UI -> audio) — short MIDI messages only (CC, PC, short notes)
    struct InjectSlot
    {
        uint8_t bytes[3] = {0, 0, 0};
        int byteCount = 0;
    };
    static constexpr int injectCapacity = 64;
    juce::AbstractFifo injectFifo { injectCapacity };
    std::array<InjectSlot, injectCapacity> injectRing {};

    // Reusable scratch buffer for filtered MIDI; clear()ed and reused per block
    // to avoid heap allocation in the audio callback.
    juce::MidiBuffer scratchBuffer;

    static float normalisedCc(int value, int ccMin, int ccMax, Curve curve);
    static float scaleToParam(float t, float paramMin, float paramMax,
                              float defaultMin, float defaultMax);

    static void copyBlockId(std::array<char, 40>& dst, const juce::String& src);

    std::vector<Mapping> mappings;
    juce::SpinLock mappingsLock;

    // Learn state — atomic-ish coordination between audio thread (reads) and
    // message thread (writes). The learnTarget/learnBlockId fields are written
    // by the message thread before learning is set true; the audio thread
    // observes them only when learning is true.
    std::atomic<bool> learning { false };
    Target learnTarget = Target::blockMix;
    juce::String learnBlockId;
    int learnTargetIndex = -1;
    int   learnCcMin    = 0;
    int   learnCcMax    = 127;
    float learnParamMin = std::numeric_limits<float>::quiet_NaN();
    float learnParamMax = std::numeric_limits<float>::quiet_NaN();
    Curve learnCurve    = Curve::Linear;
};
