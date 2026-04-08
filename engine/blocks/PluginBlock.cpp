#include "PluginBlock.h"

namespace stellarr
{

void PluginBlock::setPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin,
                            const juce::String& identifier)
{
    pluginReady.store(false, std::memory_order_release);
    stopTimer();

    pluginWindow = nullptr;
    pluginMissing = false;
    missingPluginName.clear();

    // prepareToPlay may already have been called by the caller (pre-load path).
    // Call it here as well for the single-block swap path (plugin picker).
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

    // Mark ready immediately — prepareToPlay is complete, the plugin can process.
    if (plugin != nullptr)
        pluginReady.store(true, std::memory_order_release);
}

void PluginBlock::openPluginEditor()
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

PluginBlockState PluginBlock::captureCurrentState() const
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

void PluginBlock::applyState(const PluginBlockState& s, bool suspend)
{
    setMix(s.mix);
    setBalance(s.balance);
    setLevelDb(s.levelDb);
    setBypassed(s.bypassed);
    setBypassMode(s.bypassMode);

    // Cold path (preset restore): gate the audio thread via readiness flag
    // while we apply state. Hot path (scene recall): spinlock is sufficient.
    if (suspend)
        pluginReady.store(false, std::memory_order_release);

    {
        juce::SpinLock::ScopedLockType lock(pluginLock);
        if (plugin != nullptr && s.pluginStateBase64.isNotEmpty())
        {
            juce::MemoryBlock mb;
            mb.fromBase64Encoding(s.pluginStateBase64);
            plugin->setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));
        }
    }

    if (suspend)
        pluginReady.store(true, std::memory_order_release);
}

bool PluginBlock::addState()
{
    if (static_cast<int>(states.size()) >= maxStates)
        return false;

    if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
        states[static_cast<size_t>(activeStateIndex)] = captureCurrentState();

    states.push_back(captureCurrentState());
    activeStateIndex = static_cast<int>(states.size()) - 1;
    return true;
}

bool PluginBlock::recallState(int index)
{
    if (index < 0 || index >= static_cast<int>(states.size()))
        return false;

    if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
        states[static_cast<size_t>(activeStateIndex)] = captureCurrentState();

    activeStateIndex = index;
    applyState(states[static_cast<size_t>(index)], /* suspend */ false);
    return true;
}

bool PluginBlock::deleteState(int index)
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

    applyState(states[static_cast<size_t>(activeStateIndex)], /* suspend */ false);
    return true;
}

juce::var PluginBlock::toJson() const
{
    // Save live settings into the active state before serializing
    if (activeStateIndex >= 0 && activeStateIndex < static_cast<int>(states.size()))
        states[static_cast<size_t>(activeStateIndex)] = captureCurrentState();

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

void PluginBlock::fromJson(const juce::var& json)
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

void PluginBlock::restorePluginState()
{
    pluginReady.store(false, std::memory_order_release);

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

    pluginReady.store(true, std::memory_order_release);
}

} // namespace stellarr
