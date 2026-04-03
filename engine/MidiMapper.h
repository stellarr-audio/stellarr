#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
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

private:

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
