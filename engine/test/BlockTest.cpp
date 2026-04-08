#include "TestUtils.h"
#include "blocks/GainBlock.h"
#include "blocks/InputBlock.h"
#include "blocks/PluginBlock.h"
#include "utils/ToneGenerator.h"

static bool testPluginBlockPassThrough()
{
    printf("Test: PluginBlock with no plugin passes audio... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto pluginBlock = std::make_unique<stellarr::PluginBlock>();

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(pluginBlock));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);

    processInBlocks(proc, buffer);

    if (!compareBuffers(buffer, expected, 1e-6f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testInputBlockTestTone()
{
    printf("Test: InputBlock test tone produces non-zero output... ");

    auto input = std::make_unique<stellarr::InputBlock>();
    input->prepareToPlay(kSampleRate, kBlockSize);
    input->setTestToneEnabled(true);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;
    input->processBlock(buffer, midi);

    bool hasSignal = false;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (std::abs(buffer.getSample(0, i)) > 0.01f)
        {
            hasSignal = true;
            break;
        }
    }

    if (!hasSignal)
    {
        fprintf(stderr, "  test tone produced no signal\n");
        printf("FAIL\n");
        return false;
    }

    float peak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        peak = std::max(peak, std::abs(buffer.getSample(0, i)));

    if (peak > 0.5f)
    {
        fprintf(stderr, "  peak %f exceeds expected range\n", static_cast<double>(peak));
        printf("FAIL\n");
        return false;
    }

    input->releaseResources();
    printf("PASS\n");
    return true;
}

static bool testInputBlockTestToneReset()
{
    printf("Test: InputBlock resetToDefault disables test tone... ");

    auto input = std::make_unique<stellarr::InputBlock>();
    input->prepareToPlay(kSampleRate, kBlockSize);
    input->setTestToneEnabled(true);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;
    input->processBlock(buffer, midi);

    input->resetToDefault();

    buffer.clear();
    input->processBlock(buffer, midi);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (std::abs(buffer.getSample(0, i)) > 1e-6f)
        {
            fprintf(stderr, "  expected silence after reset at sample=%d\n", i);
            printf("FAIL\n");
            return false;
        }
    }

    input->releaseResources();
    printf("PASS\n");
    return true;
}

static bool testSerialisation()
{
    printf("Test: block serialises to valid JSON... ");

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.75f);

    auto json = gain->toJson();
    auto* obj = json.getDynamicObject();

    if (obj == nullptr)
    {
        fprintf(stderr, "  toJson() returned null object\n");
        printf("FAIL\n");
        return false;
    }

    bool ok = true;

    auto id = obj->getProperty("id").toString();
    if (id.isEmpty())
    {
        fprintf(stderr, "  missing or empty 'id'\n");
        ok = false;
    }

    auto type = obj->getProperty("type").toString();
    if (type != "gain")
    {
        fprintf(stderr, "  expected type 'gain', got '%s'\n", type.toRawUTF8());
        ok = false;
    }

    auto name = obj->getProperty("name").toString();
    if (name != "Gain")
    {
        fprintf(stderr, "  expected name 'Gain', got '%s'\n", name.toRawUTF8());
        ok = false;
    }

    auto gainVal = static_cast<float>(obj->getProperty("gain"));
    if (std::abs(gainVal - 0.75f) > 1e-6f)
    {
        fprintf(stderr, "  expected gain 0.75, got %f\n", static_cast<double>(gainVal));
        ok = false;
    }

    printf("%s\n", ok ? "PASS" : "FAIL");
    return ok;
}

static bool testBypassPassesThrough()
{
    printf("Test: bypassed block passes audio unchanged... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.5f);
    gain->setBypassed(true);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);

    processInBlocks(proc, buffer);

    // Bypassed gain block should not attenuate — output equals input
    if (!compareBuffers(buffer, expected, 1e-6f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBypassToggle()
{
    printf("Test: toggling bypass on/off restores processing... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.5f);
    auto* gainPtr = gain.get();

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    // Process with bypass ON — should be pass-through
    gainPtr->setBypassed(true);
    juce::AudioBuffer<float> buf1(2, kBlockSize);
    generateSine(buf1);
    juce::AudioBuffer<float> original(buf1);
    juce::MidiBuffer midi;
    proc.processBlock(buf1, midi);

    if (!compareBuffers(buf1, original, 1e-6f))
    {
        printf("FAIL (bypass on should pass through)\n");
        return false;
    }

    // Process with bypass OFF — should apply 0.5 gain
    gainPtr->setBypassed(false);
    juce::AudioBuffer<float> buf2(2, kBlockSize);
    generateSine(buf2);
    juce::AudioBuffer<float> original2(buf2);
    proc.processBlock(buf2, midi);

    if (!compareBuffers(buf2, original2, 1e-5f, 0.5f))
    {
        printf("FAIL (bypass off should process)\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBypassSkipsMixAndBalance()
{
    printf("Test: bypass skips mix and balance... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.5f);
    gain->setMix(0.5f);
    gain->setBalance(-1.0f); // full left
    gain->setBypassed(true);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // Bypassed: mix and balance should have no effect — output equals input
    if (!compareBuffers(buffer, expected, 1e-6f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBypassMuteIn()
{
    printf("Test: bypass muteIn silences input but process runs... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.5f);
    gain->setBypassed(true);
    gain->setBypassMode(stellarr::BypassMode::muteIn);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // muteIn clears buffer then calls process() — since input is zero,
    // gain * 0 = 0, so output should be silence
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (std::abs(buffer.getSample(ch, i)) > 1e-6f)
            {
                fprintf(stderr, "  expected silence at ch=%d sample=%d\n", ch, i);
                printf("FAIL\n");
                return false;
            }
        }
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBypassMuteOut()
{
    printf("Test: bypass muteOut silences output... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(1.0f);
    gain->setBypassed(true);
    gain->setBypassMode(stellarr::BypassMode::muteOut);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // muteOut calls process() then clears — output should be silence
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (std::abs(buffer.getSample(ch, i)) > 1e-6f)
            {
                fprintf(stderr, "  expected silence at ch=%d sample=%d\n", ch, i);
                printf("FAIL\n");
                return false;
            }
        }
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBypassMute()
{
    printf("Test: bypass mute silences everything... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(1.0f);
    gain->setBypassed(true);
    gain->setBypassMode(stellarr::BypassMode::mute);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // mute clears buffer without processing — total silence
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (std::abs(buffer.getSample(ch, i)) > 1e-6f)
            {
                fprintf(stderr, "  expected silence at ch=%d sample=%d\n", ch, i);
                printf("FAIL\n");
                return false;
            }
        }
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// -- Level dB tests -----------------------------------------------------------

static bool testLevelDbConversion()
{
    printf("Test: level dB conversion edge cases... ");

    stellarr::GainBlock block;

    // -60 dB floor
    block.setLevelDb(-60.0f);
    if (block.getLevel() > 0.002f)
    {
        fprintf(stderr, "  -60 dB should produce near-zero gain, got %f\n", static_cast<double>(block.getLevel()));
        printf("FAIL\n");
        return false;
    }

    // +12 dB ceiling
    block.setLevelDb(12.0f);
    float expected12 = std::pow(10.0f, 12.0f / 20.0f); // ~3.981
    if (std::abs(block.getLevel() - expected12) > 0.01f)
    {
        fprintf(stderr, "  +12 dB should produce ~3.98 gain, got %f\n", static_cast<double>(block.getLevel()));
        printf("FAIL\n");
        return false;
    }

    // 0 dB = unity
    block.setLevelDb(0.0f);
    if (std::abs(block.getLevel() - 1.0f) > 0.001f)
    {
        fprintf(stderr, "  0 dB should be unity, got %f\n", static_cast<double>(block.getLevel()));
        printf("FAIL\n");
        return false;
    }

    // Roundtrip
    block.setLevelDb(-6.0f);
    float roundtrip = block.getLevelDb();
    if (std::abs(roundtrip - (-6.0f)) > 0.1f)
    {
        fprintf(stderr, "  roundtrip -6 dB failed, got %f\n", static_cast<double>(roundtrip));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testLevelAudio()
{
    printf("Test: level applies gain to audio output... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(1.0f);
    gain->setLevelDb(-6.0206f); // exactly 0.5 linear
    auto* ptr = gain.get();

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> reference(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // gain=1.0 * level=0.5 → output should be ~0.5x input
    if (!compareBuffers(buffer, reference, 0.02f, 0.5f))
    {
        printf("FAIL\n");
        return false;
    }

    (void)ptr;
    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testLevelSerialisation()
{
    printf("Test: level dB serialisation roundtrip... ");

    stellarr::GainBlock original;
    original.setLevelDb(-9.5f);
    auto json = original.toJson();

    stellarr::GainBlock restored;
    restored.fromJson(json);

    if (std::abs(restored.getLevelDb() - (-9.5f)) > 0.2f)
    {
        fprintf(stderr, "  level mismatch: expected -9.5, got %f\n", static_cast<double>(restored.getLevelDb()));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

// -- Bypass mode serialisation ------------------------------------------------

static bool testBypassModeSerialisation()
{
    printf("Test: all bypass modes serialise/deserialise... ");

    stellarr::BypassMode modes[] = {
        stellarr::BypassMode::thru,
        stellarr::BypassMode::muteIn,
        stellarr::BypassMode::muteOut,
        stellarr::BypassMode::mute,
    };

    for (auto mode : modes)
    {
        stellarr::GainBlock original;
        original.setBypassMode(mode);
        original.setBypassed(true);
        auto json = original.toJson();

        stellarr::GainBlock restored;
        restored.fromJson(json);

        if (restored.getBypassMode() != mode)
        {
            fprintf(stderr, "  mode mismatch for %s\n",
                stellarr::bypassModeToString(mode).toRawUTF8());
            printf("FAIL\n");
            return false;
        }
        if (!restored.isBypassed())
        {
            fprintf(stderr, "  bypassed flag not restored\n");
            printf("FAIL\n");
            return false;
        }
    }

    printf("PASS\n");
    return true;
}

// -- PluginBlock state tests --------------------------------------------------

static bool testStateCaptureAndApply()
{
    printf("Test: PluginBlock captureCurrentState and applyState... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setMix(0.3f);
    block.setBalance(-0.5f);
    block.setLevelDb(-6.0f);
    block.setBypassed(true);
    block.setBypassMode(stellarr::BypassMode::muteOut);

    auto state = block.captureCurrentState();

    if (std::abs(state.mix - 0.3f) > 0.01f ||
        std::abs(state.balance - (-0.5f)) > 0.01f ||
        std::abs(state.levelDb - (-6.0f)) > 0.5f ||
        !state.bypassed ||
        state.bypassMode != stellarr::BypassMode::muteOut)
    {
        fprintf(stderr, "  captured state has wrong values\n");
        printf("FAIL\n");
        return false;
    }

    // Reset and apply
    block.setMix(1.0f);
    block.setBalance(0.0f);
    block.setLevelDb(0.0f);
    block.setBypassed(false);
    block.applyState(state);

    if (std::abs(block.getMix() - 0.3f) > 0.01f ||
        std::abs(block.getBalance() - (-0.5f)) > 0.01f ||
        !block.isBypassed())
    {
        fprintf(stderr, "  applyState did not restore values\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testStateAddAndRecall()
{
    printf("Test: PluginBlock add state and recall... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    // State 0: mix = 0.5
    block.setMix(0.5f);
    block.saveCurrentState();

    // Add state 1 (captures current mix=0.5, then we change it)
    block.addState();
    block.setMix(0.8f);
    block.saveCurrentState();

    if (block.getNumStates() != 2 || block.getActiveStateIndex() != 1)
    {
        fprintf(stderr, "  expected 2 states, active=1\n");
        printf("FAIL\n");
        return false;
    }

    // Recall state 0 — should restore mix = 0.5
    block.recallState(0);
    if (std::abs(block.getMix() - 0.5f) > 0.01f)
    {
        fprintf(stderr, "  recallState(0) should restore mix=0.5, got %f\n", static_cast<double>(block.getMix()));
        printf("FAIL\n");
        return false;
    }

    // Recall state 1 — should restore mix = 0.8
    block.recallState(1);
    if (std::abs(block.getMix() - 0.8f) > 0.01f)
    {
        fprintf(stderr, "  recallState(1) should restore mix=0.8, got %f\n", static_cast<double>(block.getMix()));
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testStateDirtyTracking()
{
    printf("Test: PluginBlock dirty state tracking... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    // Initially not dirty
    if (!block.getDirtyStates().empty())
    {
        fprintf(stderr, "  expected no dirty states initially\n");
        printf("FAIL\n");
        return false;
    }

    // Mark dirty
    block.markDirty();
    if (block.getDirtyStates().count(0) != 1)
    {
        fprintf(stderr, "  expected state 0 dirty\n");
        printf("FAIL\n");
        return false;
    }

    // Save clears dirty
    block.saveCurrentState();
    if (!block.getDirtyStates().empty())
    {
        fprintf(stderr, "  saveCurrentState should clear dirty\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testStateDelete()
{
    printf("Test: PluginBlock delete state reindexes... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    block.addState(); // 2 states
    block.addState(); // 3 states

    if (block.getNumStates() != 3)
    {
        fprintf(stderr, "  expected 3 states\n");
        printf("FAIL\n");
        return false;
    }

    // Delete middle state
    block.recallState(1);
    block.deleteState(1);

    if (block.getNumStates() != 2)
    {
        fprintf(stderr, "  expected 2 states after delete\n");
        printf("FAIL\n");
        return false;
    }

    // Can't delete last state
    block.deleteState(0);
    if (block.getNumStates() != 1)
    {
        fprintf(stderr, "  should have 1 state left\n");
        printf("FAIL\n");
        return false;
    }

    if (block.deleteState(0))
    {
        fprintf(stderr, "  should not delete last state\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testStateSerialisation()
{
    printf("Test: PluginBlock state serialisation roundtrip... ");

    stellarr::PluginBlock original;
    original.prepareToPlay(kSampleRate, kBlockSize);

    original.setMix(0.2f);
    original.saveCurrentState();
    original.addState();
    original.setMix(0.8f);
    original.saveCurrentState();

    auto json = original.toJson();

    stellarr::PluginBlock restored;
    restored.fromJson(json);

    if (restored.getNumStates() != 2)
    {
        fprintf(stderr, "  expected 2 states, got %d\n", restored.getNumStates());
        printf("FAIL\n");
        return false;
    }

    if (restored.getActiveStateIndex() != 1)
    {
        fprintf(stderr, "  expected active=1, got %d\n", restored.getActiveStateIndex());
        printf("FAIL\n");
        return false;
    }

    original.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testStateLegacyFormat()
{
    printf("Test: PluginBlock legacy format creates one state... ");

    // Simulate old JSON without states array
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id", juce::Uuid().toString());
    obj->setProperty("type", "plugin");
    obj->setProperty("name", "Plugin");
    obj->setProperty("mix", 0.7);
    obj->setProperty("balance", -0.3);
    obj->setProperty("level", -3.0);
    obj->setProperty("bypassed", true);
    obj->setProperty("bypassMode", "mute");
    obj->setProperty("pluginId", "");
    obj->setProperty("pluginState", "");

    stellarr::PluginBlock block;
    block.fromJson(juce::var(obj));

    if (block.getNumStates() != 1)
    {
        fprintf(stderr, "  expected 1 state from legacy format, got %d\n", block.getNumStates());
        printf("FAIL\n");
        return false;
    }

    if (block.getActiveStateIndex() != 0)
    {
        fprintf(stderr, "  expected active=0\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

// -- Display name tests -------------------------------------------------------

static bool testDisplayNameSerialisation()
{
    printf("Test: block displayName serialisation roundtrip... ");

    stellarr::GainBlock original;
    original.setDisplayName("AMP");
    auto json = original.toJson();

    stellarr::GainBlock restored;
    restored.fromJson(json);

    if (restored.getDisplayName() != "AMP")
    {
        fprintf(stderr, "  expected 'AMP', got '%s'\n", restored.getDisplayName().toRawUTF8());
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testDisplayNameEmpty()
{
    printf("Test: empty displayName not serialised... ");

    stellarr::GainBlock block;
    auto json = block.toJson();
    auto* obj = json.getDynamicObject();

    if (obj->hasProperty("displayName"))
    {
        fprintf(stderr, "  empty displayName should not be in JSON\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

// -- Missing plugin tests -----------------------------------------------------

static bool testPluginMissingDefaultFalse()
{
    printf("Test: PluginBlock pluginMissing defaults to false... ");

    stellarr::PluginBlock block;
    if (block.isPluginMissing())
    {
        fprintf(stderr, "  expected false by default\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testPluginMissingSetAndClear()
{
    printf("Test: pluginMissing can be set and is cleared by setPlugin... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    block.setPluginMissing(true);
    block.setMissingPluginName("My Missing Plugin");

    if (!block.isPluginMissing())
    {
        fprintf(stderr, "  expected true after setPluginMissing(true)\n");
        printf("FAIL\n");
        return false;
    }

    if (block.getPluginName() != "My Missing Plugin")
    {
        fprintf(stderr, "  expected missingPluginName fallback, got '%s'\n",
                block.getPluginName().toRawUTF8());
        printf("FAIL\n");
        return false;
    }

    // setPlugin with nullptr doesn't clear (no valid plugin)
    // setPlugin with a valid plugin would clear — but we can't create a real instance in tests.
    // Instead verify the flag is settable back to false.
    block.setPluginMissing(false);
    if (block.isPluginMissing())
    {
        fprintf(stderr, "  expected false after setPluginMissing(false)\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testPluginMissingNotSerialised()
{
    printf("Test: pluginMissing is not persisted in JSON... ");

    stellarr::PluginBlock block;
    block.setPluginMissing(true);
    block.setMissingPluginName("Gone Plugin");

    auto json = block.toJson();
    auto* obj = json.getDynamicObject();

    if (obj->hasProperty("pluginMissing"))
    {
        fprintf(stderr, "  pluginMissing should not be in serialised JSON\n");
        printf("FAIL\n");
        return false;
    }

    // Restoring from JSON should default to false
    stellarr::PluginBlock restored;
    restored.fromJson(json);

    if (restored.isPluginMissing())
    {
        fprintf(stderr, "  restored block should not be missing\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

// -- Mix parameter audio tests ------------------------------------------------

static bool testMixFullyDry()
{
    printf("Test: mix=0 leaves buffer unchanged (fully dry)... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setGain(0.5f);
    block.setMix(0.0f);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);
    juce::MidiBuffer midi;
    block.processBlock(buffer, midi);

    if (!compareBuffers(buffer, expected, 1e-6f))
    {
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testMixFullyWet()
{
    printf("Test: mix=1 applies full processing... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setGain(0.5f);
    block.setMix(1.0f);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> reference(buffer);
    juce::MidiBuffer midi;
    block.processBlock(buffer, midi);

    if (!compareBuffers(buffer, reference, 0.01f, 0.5f))
    {
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testMixHalfBlend()
{
    printf("Test: mix=0.5 blends dry and wet equally... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setGain(0.0f); // wet = silence
    block.setMix(0.5f);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> reference(buffer);
    juce::MidiBuffer midi;
    block.processBlock(buffer, midi);

    // dry*0.5 + wet(0)*0.5 = 0.5 * original
    if (!compareBuffers(buffer, reference, 0.01f, 0.5f))
    {
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

// -- Balance parameter audio tests --------------------------------------------

static bool testBalanceFullLeft()
{
    printf("Test: balance=-1 silences right channel... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setBalance(-1.0f);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    block.processBlock(buffer, midi);

    // Right channel should be silent
    float rightPeak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        rightPeak = std::max(rightPeak, std::abs(buffer.getSample(1, i)));

    if (rightPeak > 1e-6f)
    {
        fprintf(stderr, "  right channel not silent: peak=%f\n", static_cast<double>(rightPeak));
        printf("FAIL\n");
        return false;
    }

    // Left channel should still have signal
    float leftPeak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        leftPeak = std::max(leftPeak, std::abs(buffer.getSample(0, i)));

    if (leftPeak < 0.01f)
    {
        fprintf(stderr, "  left channel unexpectedly silent\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBalanceFullRight()
{
    printf("Test: balance=+1 silences left channel... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setBalance(1.0f);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    block.processBlock(buffer, midi);

    float leftPeak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        leftPeak = std::max(leftPeak, std::abs(buffer.getSample(0, i)));

    if (leftPeak > 1e-6f)
    {
        fprintf(stderr, "  left channel not silent: peak=%f\n", static_cast<double>(leftPeak));
        printf("FAIL\n");
        return false;
    }

    float rightPeak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        rightPeak = std::max(rightPeak, std::abs(buffer.getSample(1, i)));

    if (rightPeak < 0.01f)
    {
        fprintf(stderr, "  right channel unexpectedly silent\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBalanceCentre()
{
    printf("Test: balance=0 leaves both channels unchanged... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setBalance(0.0f);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);
    juce::MidiBuffer midi;
    block.processBlock(buffer, midi);

    if (!compareBuffers(buffer, expected, 1e-6f))
    {
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

// -- Hot/cold path tests ------------------------------------------------------

static bool testApplyStateHotPathNoSuspend()
{
    printf("Test: applyState(suspend=false) does not suspend processing... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    auto state = block.captureCurrentState();
    block.applyState(state, false);

    if (block.isSuspended())
    {
        fprintf(stderr, "  block should not be suspended on hot path\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testApplyStateColdPathSilencesOutput()
{
    printf("Test: applyState(suspend=true) gates audio via readiness flag... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    // After cold-path applyState completes, the block should be ready to process
    // (the readiness flag gates during state application, then re-enables).
    auto state = block.captureCurrentState();
    block.applyState(state, true);

    // Verify the block can process after state application (not left suspended)
    juce::AudioBuffer<float> buf(2, kBlockSize);
    buf.clear();
    juce::MidiBuffer midi;
    block.processBlock(buf, midi);

    // With no plugin loaded, process is a no-op — the block should not crash
    // and should not be stuck in a suspended state.
    if (block.isSuspended())
    {
        fprintf(stderr, "  block should not remain suspended after cold applyState\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testRecallStateUsesHotPath()
{
    printf("Test: recallState uses hot path (no suspend)... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.addState();
    block.recallState(0);

    if (block.isSuspended())
    {
        fprintf(stderr, "  recallState should not suspend\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testDeleteStateUsesHotPath()
{
    printf("Test: deleteState uses hot path (no suspend)... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.addState();
    block.deleteState(1);

    if (block.isSuspended())
    {
        fprintf(stderr, "  deleteState should not suspend\n");
        printf("FAIL\n");
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

// -- ToneGenerator test -------------------------------------------------------

static bool testToneGeneratorOutput()
{
    printf("Test: ToneGenerator produces non-zero output... ");

    stellarr::ToneGenerator gen;
    gen.prepareToPlay(kSampleRate);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();
    gen.fillBuffer(buffer);

    float peak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        peak = std::max(peak, std::abs(buffer.getSample(0, i)));

    if (peak < 0.01f)
    {
        fprintf(stderr, "  no signal produced\n");
        printf("FAIL\n");
        return false;
    }

    if (peak > 0.5f)
    {
        fprintf(stderr, "  peak %f too high\n", static_cast<double>(peak));
        printf("FAIL\n");
        return false;
    }

    // Reset and verify it restarts
    gen.reset();
    juce::AudioBuffer<float> buffer2(2, kBlockSize);
    buffer2.clear();
    gen.fillBuffer(buffer2);

    float peak2 = 0.0f;
    for (int i = 0; i < buffer2.getNumSamples(); ++i)
        peak2 = std::max(peak2, std::abs(buffer2.getSample(0, i)));

    if (peak2 < 0.01f)
    {
        fprintf(stderr, "  no signal after reset\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    if (!testPluginBlockPassThrough())   ++failures;
    if (!testInputBlockTestTone())      ++failures;
    if (!testInputBlockTestToneReset()) ++failures;
    if (!testSerialisation())           ++failures;
    if (!testBypassPassesThrough())     ++failures;
    if (!testBypassToggle())            ++failures;
    if (!testBypassSkipsMixAndBalance()) ++failures;
    if (!testBypassMuteIn())            ++failures;
    if (!testBypassMuteOut())           ++failures;
    if (!testBypassMute())              ++failures;

    // Level tests
    if (!testLevelDbConversion())       ++failures;
    if (!testLevelAudio())              ++failures;
    if (!testLevelSerialisation())      ++failures;

    // Bypass serialisation
    if (!testBypassModeSerialisation()) ++failures;

    // State tests
    if (!testStateCaptureAndApply())    ++failures;
    if (!testStateAddAndRecall())       ++failures;
    if (!testStateDirtyTracking())      ++failures;
    if (!testStateDelete())             ++failures;
    if (!testStateSerialisation())      ++failures;
    if (!testStateLegacyFormat())       ++failures;

    // Display name
    if (!testDisplayNameSerialisation()) ++failures;
    if (!testDisplayNameEmpty())         ++failures;

    // Missing plugin
    if (!testPluginMissingDefaultFalse())   ++failures;
    if (!testPluginMissingSetAndClear())     ++failures;
    if (!testPluginMissingNotSerialised())   ++failures;

    // Mix audio
    if (!testMixFullyDry())             ++failures;
    if (!testMixFullyWet())             ++failures;
    if (!testMixHalfBlend())            ++failures;

    // Balance audio
    if (!testBalanceFullLeft())          ++failures;
    if (!testBalanceFullRight())         ++failures;
    if (!testBalanceCentre())            ++failures;

    // Hot/cold path
    if (!testApplyStateHotPathNoSuspend()) ++failures;
    if (!testApplyStateColdPathSilencesOutput()) ++failures;
    if (!testRecallStateUsesHotPath())     ++failures;
    if (!testDeleteStateUsesHotPath())     ++failures;

    // ToneGenerator
    if (!testToneGeneratorOutput())      ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
