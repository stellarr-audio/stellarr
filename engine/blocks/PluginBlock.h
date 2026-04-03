#pragma once
#include "Block.h"
#include "../PluginWindow.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <set>

namespace stellarr
{

struct PluginBlockState
{
    juce::String pluginStateBase64;
    float mix = 1.0f;
    float balance = 0.0f;
    float levelDb = 0.0f;
    bool bypassed = false;
    BypassMode bypassMode = BypassMode::thru;
};

class PluginBlock final : public Block
{
public:
    PluginBlock() : Block("Plugin", 2, 2)
    {
        // Start with one default state
        states.push_back({});
    }

    BlockType getBlockType() const override { return BlockType::plugin; }

    void prepareBlock(double sampleRate, int samplesPerBlock) override
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;

        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr)
            plugin->prepareToPlay(sampleRate, samplesPerBlock);
    }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        juce::SpinLock::ScopedTryLockType lock(pluginLock);

        if (lock.isLocked() && plugin != nullptr
            && plugin->getTotalNumInputChannels() <= buffer.getNumChannels()
            && plugin->getTotalNumOutputChannels() <= buffer.getNumChannels())
        {
            plugin->processBlock(buffer, midi);
        }
    }

    void setPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin,
                   const juce::String& identifier)
    {
        pluginWindow = nullptr;

        if (newPlugin != nullptr)
        {
            newPlugin->setPlayConfigDetails(
                getTotalNumInputChannels(), getTotalNumOutputChannels(),
                currentSampleRate, currentBlockSize);
            newPlugin->prepareToPlay(currentSampleRate, currentBlockSize);
        }

        {
            juce::SpinLock::ScopedLockType lock(pluginLock);
            std::swap(plugin, newPlugin);
            pluginIdentifier = identifier;
        }

        if (newPlugin != nullptr)
            newPlugin->releaseResources();
    }

    juce::String getPluginIdentifier() const { return pluginIdentifier; }

    juce::String getPluginName() const
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr)
            return plugin->getName();
        return {};
    }

    juce::String getPluginFormat() const
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr)
            return plugin->getPluginDescription().pluginFormatName;
        return {};
    }

    bool hasPlugin() const
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        return plugin != nullptr;
    }

    void openPluginEditor()
    {
        juce::AudioPluginInstance* p = nullptr;
        juce::String name;

        {
            juce::SpinLock::ScopedLockType lock(pluginLock);
            if (plugin == nullptr) return;
            p = plugin.get();
            name = plugin->getName();
        }

        if (pluginWindow != nullptr)
        {
            if (pluginWindow->isVisible())
            {
                pluginWindow->toFront(true);
                return;
            }
            pluginWindow = nullptr;
        }

        if (p->hasEditor())
        {
            if (auto* editor = p->createEditorAndMakeActive())
                pluginWindow = std::make_unique<PluginWindow>(editor, name);
            else
                pluginWindow = std::make_unique<PluginWindow>(
                    new juce::GenericAudioProcessorEditor(*p), name);
        }
        else
        {
            pluginWindow = std::make_unique<PluginWindow>(
                new juce::GenericAudioProcessorEditor(*p), name);
        }
    }

    void closePluginEditor()
    {
        pluginWindow = nullptr;
    }

    // -- State management -----------------------------------------------------

    static constexpr int maxStates = 16;

    int getNumStates() const { return static_cast<int>(states.size()); }
    int getActiveStateIndex() const { return activeStateIndex; }
    const std::set<int>& getDirtyStates() const { return dirtyStates; }
    void markDirty() { dirtyStates.insert(activeStateIndex); }

    PluginBlockState captureCurrentState() const
    {
        PluginBlockState s;
        s.mix = getMix();
        s.balance = getBalance();
        s.levelDb = getLevelDb();
        s.bypassed = isBypassed();
        s.bypassMode = getBypassMode();

        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr)
        {
            juce::MemoryBlock mb;
            plugin->getStateInformation(mb);
            s.pluginStateBase64 = mb.toBase64Encoding();
        }

        return s;
    }

    void applyState(const PluginBlockState& s)
    {
        setMix(s.mix);
        setBalance(s.balance);
        setLevelDb(s.levelDb);
        setBypassed(s.bypassed);
        setBypassMode(s.bypassMode);

        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr && s.pluginStateBase64.isNotEmpty())
        {
            juce::MemoryBlock mb;
            mb.fromBase64Encoding(s.pluginStateBase64);
            plugin->setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));
        }
    }

    // Save all dirty states and clear dirty flags
    void saveCurrentState()
    {
        // Capture live settings into active slot
        if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
            states[static_cast<size_t>(activeStateIndex)] = captureCurrentState();

        dirtyStates.clear();
    }

    // Add a new state from current live settings (returns false if at max)
    bool addState()
    {
        if (static_cast<int>(states.size()) >= maxStates)
            return false;

        // Capture current into active slot before adding (preserve changes)
        if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
            states[static_cast<size_t>(activeStateIndex)] = captureCurrentState();

        states.push_back(captureCurrentState());
        activeStateIndex = static_cast<int>(states.size()) - 1;
        return true;
    }

    // Recall a saved state by index
    bool recallState(int index)
    {
        if (index < 0 || index >= static_cast<int>(states.size()))
            return false;

        // Capture current live settings into the outgoing slot (preserve changes)
        if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
            states[static_cast<size_t>(activeStateIndex)] = captureCurrentState();

        activeStateIndex = index;
        applyState(states[static_cast<size_t>(index)]);
        return true;
    }

    // Delete a state by index (must keep at least one)
    bool deleteState(int index)
    {
        if (states.size() <= 1 || index < 0 || index >= static_cast<int>(states.size()))
            return false;

        states.erase(states.begin() + index);
        dirtyStates.erase(index);

        // Shift dirty indices above the deleted index
        std::set<int> shifted;
        for (int d : dirtyStates)
            shifted.insert(d > index ? d - 1 : d);
        dirtyStates = shifted;

        if (activeStateIndex >= static_cast<int>(states.size()))
            activeStateIndex = static_cast<int>(states.size()) - 1;
        else if (index < activeStateIndex)
            activeStateIndex--;

        applyState(states[static_cast<size_t>(activeStateIndex)]);
        return true;
    }

    // -- Serialization --------------------------------------------------------

    juce::var toJson() const override
    {
        // Save live settings into the active state before serializing
        if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
            const_cast<PluginBlock*>(this)->states[static_cast<size_t>(activeStateIndex)] = captureCurrentState();

        auto json = Block::toJson();
        if (auto* obj = json.getDynamicObject())
        {
            obj->setProperty("pluginId", pluginIdentifier);
            obj->setProperty("pluginName", getPluginName());

            // Write states array
            juce::Array<juce::var> statesArray;
            for (auto& s : states)
            {
                auto* stateObj = new juce::DynamicObject();
                stateObj->setProperty("pluginState", s.pluginStateBase64);
                stateObj->setProperty("mix", static_cast<double>(s.mix));
                stateObj->setProperty("balance", static_cast<double>(s.balance));
                stateObj->setProperty("level", static_cast<double>(s.levelDb));
                stateObj->setProperty("bypassed", s.bypassed);
                stateObj->setProperty("bypassMode", bypassModeToString(s.bypassMode));
                statesArray.add(juce::var(stateObj));
            }
            obj->setProperty("states", statesArray);
            obj->setProperty("activeStateIndex", activeStateIndex);

            // Backward compat: write top-level pluginState from active state
            if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
                obj->setProperty("pluginState", states[static_cast<size_t>(activeStateIndex)].pluginStateBase64);
        }
        return json;
    }

    void fromJson(const juce::var& json) override
    {
        Block::fromJson(json);
        if (auto* obj = json.getDynamicObject())
        {
            pluginIdentifier = obj->getProperty("pluginId").toString();

            // Try loading states array (new format)
            auto statesVar = obj->getProperty("states");
            if (auto* arr = statesVar.getArray())
            {
                states.clear();
                for (auto& sv : *arr)
                {
                    if (auto* so = sv.getDynamicObject())
                    {
                        PluginBlockState s;
                        s.pluginStateBase64 = so->getProperty("pluginState").toString();
                        s.mix = static_cast<float>(so->getProperty("mix"));
                        s.balance = static_cast<float>(so->getProperty("balance"));
                        s.levelDb = so->hasProperty("level") ? static_cast<float>(so->getProperty("level")) : 0.0f;
                        s.bypassed = static_cast<bool>(so->getProperty("bypassed"));
                        s.bypassMode = bypassModeFromString(so->getProperty("bypassMode").toString());
                        states.push_back(s);
                    }
                }
                activeStateIndex = static_cast<int>(obj->getProperty("activeStateIndex"));
                if (states.empty())
                    states.push_back({});
                if (activeStateIndex < 0 || activeStateIndex >= static_cast<int>(states.size()))
                    activeStateIndex = 0;
            }
            else
            {
                // Legacy format: single state from top-level fields
                states.clear();
                PluginBlockState s;
                s.pluginStateBase64 = obj->getProperty("pluginState").toString();
                s.mix = getMix();
                s.balance = getBalance();
                s.levelDb = getLevelDb();
                s.bypassed = isBypassed();
                s.bypassMode = getBypassMode();
                states.push_back(s);
                activeStateIndex = 0;
            }

            // Store for restorePluginState()
            if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
                pluginStateBase64 = states[static_cast<size_t>(activeStateIndex)].pluginStateBase64;
        }
    }

    // Call after the plugin instance has been loaded to restore its state
    void restorePluginState()
    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr && pluginStateBase64.isNotEmpty())
        {
            juce::MemoryBlock mb;
            mb.fromBase64Encoding(pluginStateBase64);
            plugin->setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));
            pluginStateBase64.clear();
        }
    }

private:
    mutable juce::SpinLock pluginLock;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::unique_ptr<PluginWindow> pluginWindow;
    juce::String pluginIdentifier;
    juce::String pluginStateBase64;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    std::vector<PluginBlockState> states;
    int activeStateIndex = 0;
    std::set<int> dirtyStates;
};

} // namespace stellarr
