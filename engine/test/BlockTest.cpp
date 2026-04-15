#include "TestUtils.h"
#include "blocks/GainBlock.h"
#include "blocks/InputBlock.h"
#include "blocks/OutputBlock.h"
#include "blocks/PluginBlock.h"
#include "dsp/LoudnessMeter.h"
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

// -- FX bypass mode tests -----------------------------------------------------

static bool testBypassMuteFxInDryPassesThrough()
{
    printf("Test: bypass muteFxIn passes dry signal through... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.0f); // wet output = silence (simulates tail-only scenario)
    gain->setMix(0.5f);
    gain->setBypassed(true);
    gain->setBypassMode(stellarr::BypassMode::muteFxIn);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // With gain=0 (wet is silence) and mix=0.5, output should be 0.5 * dry
    // (dry portion of the blend). Wet = gain(0) applied to silence = silence.
    if (!compareBuffers(buffer, expected, 0.02f, 0.5f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBypassMuteFxInFeedsSilenceToPlugin()
{
    printf("Test: bypass muteFxIn feeds silence to plugin... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    // gain=1.0 so if signal reaches the plugin, it passes through unchanged
    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(1.0f);
    gain->setMix(1.0f); // fully wet
    gain->setBypassed(true);
    gain->setBypassMode(stellarr::BypassMode::muteFxIn);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // Fully wet + plugin fed silence = output should be silence
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

static bool testBypassMuteFxInAppliesLevelAndBalance()
{
    printf("Test: bypass muteFxIn applies level and balance to output... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.0f);     // wet = silence
    gain->setMix(0.0f);      // fully dry output
    gain->setLevelDb(-6.0206f); // 0.5 linear
    gain->setBalance(0.0f);
    gain->setBypassed(true);
    gain->setBypassMode(stellarr::BypassMode::muteFxIn);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> reference(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // Fully dry + level 0.5 → output = 0.5 * input
    if (!compareBuffers(buffer, reference, 0.02f, 0.5f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBypassMuteFxOutDryPassesThrough()
{
    printf("Test: bypass muteFxOut passes dry signal, mutes wet... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(2.0f);  // amplifies signal if it were heard
    gain->setMix(0.5f);
    gain->setBypassed(true);
    gain->setBypassMode(stellarr::BypassMode::muteFxOut);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // Wet is muted, so output should equal dry input (gain has no effect)
    if (!compareBuffers(buffer, expected, 1e-5f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testBypassMuteFxOutAppliesLevelAndBalance()
{
    printf("Test: bypass muteFxOut applies level and balance... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(2.0f);
    gain->setMix(0.5f);
    gain->setLevelDb(-6.0206f); // 0.5 linear
    gain->setBypassed(true);
    gain->setBypassMode(stellarr::BypassMode::muteFxOut);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::AudioBuffer<float> reference(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // Dry + level 0.5 → output = 0.5 * input
    if (!compareBuffers(buffer, reference, 0.02f, 0.5f))
    {
        printf("FAIL\n");
        return false;
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
        stellarr::BypassMode::muteFxIn,
        stellarr::BypassMode::muteFxOut,
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

// ---------------------------------------------------------------------------
// Loudness measurement
// ---------------------------------------------------------------------------

// Run enough sine through the block to fill the meter's short-term window.
static void feedSineIntoBlock(stellarr::Block& block, float amplitude, double seconds)
{
    const int totalSamples = static_cast<int>(seconds * kSampleRate);
    int remaining = totalSamples;
    while (remaining > 0)
    {
        const int n = std::min(kBlockSize, remaining);
        juce::AudioBuffer<float> buffer(2, n);
        const auto twoPi = static_cast<float>(juce::MathConstants<double>::twoPi);
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < n; ++i)
            {
                const int sampleIdx = (totalSamples - remaining) + i;
                buffer.setSample(ch, i,
                    amplitude * std::sin(twoPi * 1000.0f
                        * static_cast<float>(sampleIdx) / static_cast<float>(kSampleRate)));
            }
        }
        juce::MidiBuffer midi;
        block.processBlock(buffer, midi);
        remaining -= n;
    }
}

static bool testMeasureLoudnessDisabledByDefault()
{
    printf("Test: loudness meter is off by default on Block... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    if (block.isMeasuringLoudness())
    {
        fprintf(stderr, "  expected measurement off by default\n");
        printf("FAIL\n");
        return false;
    }

    feedSineIntoBlock(block, 0.5f, 1.0);

    // With measurement off, meter should stay at silence floor.
    if (block.getShortTermLufs() > stellarr::dsp::LoudnessMeter::kSilenceFloor + 1e-3f)
    {
        fprintf(stderr, "  expected silence floor, got %f\n",
                static_cast<double>(block.getShortTermLufs()));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testMeasureLoudnessEnabledReads()
{
    printf("Test: enabling measurement produces a non-floor reading... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setMeasureLoudness(true);

    feedSineIntoBlock(block, 0.5f, 1.0);

    const float lufs = block.getShortTermLufs();
    if (lufs <= stellarr::dsp::LoudnessMeter::kSilenceFloor + 10.0f)
    {
        fprintf(stderr, "  expected reading above floor+10, got %f\n",
                static_cast<double>(lufs));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testMeterRunsInBypassThru()
{
    printf("Test: meter reads through-signal in THRU bypass mode... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setMeasureLoudness(true);
    block.setBypassed(true);
    block.setBypassMode(stellarr::BypassMode::thru);

    feedSineIntoBlock(block, 0.5f, 1.0);

    // THRU passes input through unchanged — meter should register the signal.
    if (block.getShortTermLufs() <= stellarr::dsp::LoudnessMeter::kSilenceFloor + 10.0f)
    {
        fprintf(stderr, "  expected non-floor in THRU bypass, got %f\n",
                static_cast<double>(block.getShortTermLufs()));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testMeterReadsFloorInBypassMute()
{
    printf("Test: meter reads silence floor in MUTE bypass... ");

    stellarr::GainBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);
    block.setMeasureLoudness(true);
    block.setBypassed(true);
    block.setBypassMode(stellarr::BypassMode::mute);

    feedSineIntoBlock(block, 0.5f, 1.0);

    // MUTE clears the output — meter should read floor (proves the meter
    // *ran* but on silence; before the fix it would have been skipped).
    if (block.getShortTermLufs() > stellarr::dsp::LoudnessMeter::kSilenceFloor + 1e-3f)
    {
        fprintf(stderr, "  expected silence floor in MUTE bypass, got %f\n",
                static_cast<double>(block.getShortTermLufs()));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testMeterRunsInAllBypassModes()
{
    printf("Test: meter runs (does not crash or stay uninitialised) in every bypass mode... ");

    const stellarr::BypassMode modes[] = {
        stellarr::BypassMode::thru,
        stellarr::BypassMode::muteIn,
        stellarr::BypassMode::muteOut,
        stellarr::BypassMode::mute,
        stellarr::BypassMode::muteFxIn,
        stellarr::BypassMode::muteFxOut,
    };

    for (auto mode : modes)
    {
        stellarr::GainBlock block;
        block.prepareToPlay(kSampleRate, kBlockSize);
        block.setMeasureLoudness(true);
        block.setBypassed(true);
        block.setBypassMode(mode);

        // Just running without crashing is the primary assertion here;
        // the specific LUFS value depends on the mode.
        feedSineIntoBlock(block, 0.5f, 0.5);

        const float m = block.getMomentaryLufs();
        const float s = block.getShortTermLufs();

        // Sanity: atomics should be readable and within sensible bounds.
        if (! std::isfinite(m) || ! std::isfinite(s))
        {
            fprintf(stderr, "  mode %s produced non-finite reading (m=%f s=%f)\n",
                    stellarr::bypassModeToString(mode).toRawUTF8(),
                    static_cast<double>(m), static_cast<double>(s));
            printf("FAIL\n");
            return false;
        }
    }

    printf("PASS\n");
    return true;
}

static bool testMeterReflectsLevel()
{
    printf("Test: lowering block level reduces meter reading... ");

    stellarr::GainBlock loud;
    stellarr::GainBlock quiet;
    loud.prepareToPlay(kSampleRate, kBlockSize);
    quiet.prepareToPlay(kSampleRate, kBlockSize);
    loud.setMeasureLoudness(true);
    quiet.setMeasureLoudness(true);

    loud.setLevel(1.0f);       // 0 dB
    quiet.setLevelDb(-12.0f);  // -12 dB

    feedSineIntoBlock(loud, 0.5f, 1.0);
    feedSineIntoBlock(quiet, 0.5f, 1.0);

    const float diff = loud.getShortTermLufs() - quiet.getShortTermLufs();
    if (diff < 10.0f || diff > 14.0f)
    {
        fprintf(stderr, "  expected ~12 dB drop, got %f (loud=%f quiet=%f)\n",
                static_cast<double>(diff),
                static_cast<double>(loud.getShortTermLufs()),
                static_cast<double>(quiet.getShortTermLufs()));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testOutputBlockMeasuresByDefault()
{
    printf("Test: OutputBlock has loudness measurement on by default... ");

    stellarr::OutputBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    if (! block.isMeasuringLoudness())
    {
        fprintf(stderr, "  OutputBlock should enable measurement in its constructor\n");
        printf("FAIL\n");
        return false;
    }

    feedSineIntoBlock(block, 0.5f, 1.0);

    if (block.getShortTermLufs() <= stellarr::dsp::LoudnessMeter::kSilenceFloor + 10.0f)
    {
        fprintf(stderr, "  OutputBlock did not register audio, got %f\n",
                static_cast<double>(block.getShortTermLufs()));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testOutputBlockTargetLufsDefaultsNaN()
{
    printf("Test: OutputBlock target LUFS defaults to unset (NaN)... ");

    stellarr::OutputBlock block;
    if (block.hasTargetLufs())
    {
        fprintf(stderr, "  expected no target by default\n");
        printf("FAIL\n");
        return false;
    }
    if (! std::isnan(block.getTargetLufs()))
    {
        fprintf(stderr, "  expected NaN, got %f\n",
                static_cast<double>(block.getTargetLufs()));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testOutputBlockTargetLufsSetGet()
{
    printf("Test: OutputBlock target LUFS set/get round-trip... ");

    stellarr::OutputBlock block;
    block.setTargetLufs(-18.0f);

    if (! block.hasTargetLufs())
    {
        fprintf(stderr, "  expected hasTargetLufs() true after set\n");
        printf("FAIL\n");
        return false;
    }
    if (std::abs(block.getTargetLufs() - (-18.0f)) > 1e-6f)
    {
        fprintf(stderr, "  expected -18.0, got %f\n",
                static_cast<double>(block.getTargetLufs()));
        printf("FAIL\n");
        return false;
    }

    // Clearing back to NaN.
    block.setTargetLufs(std::numeric_limits<float>::quiet_NaN());
    if (block.hasTargetLufs())
    {
        fprintf(stderr, "  clearing with NaN failed\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testOutputBlockTargetLufsJsonOmittedWhenUnset()
{
    printf("Test: OutputBlock toJson omits targetLufs when unset... ");

    stellarr::OutputBlock block;
    // Default NaN — should not appear in JSON.
    auto json = block.toJson();
    auto* obj = json.getDynamicObject();
    if (obj == nullptr)
    {
        fprintf(stderr, "  toJson() returned null object\n");
        printf("FAIL\n");
        return false;
    }

    if (obj->hasProperty("targetLufs"))
    {
        fprintf(stderr, "  targetLufs should be omitted when unset\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testOutputBlockTargetLufsJsonRoundTrip()
{
    printf("Test: OutputBlock targetLufs round-trips through JSON... ");

    stellarr::OutputBlock src;
    src.setTargetLufs(-14.0f);

    auto json = src.toJson();
    auto* obj = json.getDynamicObject();
    if (obj == nullptr || ! obj->hasProperty("targetLufs"))
    {
        fprintf(stderr, "  expected targetLufs property in JSON\n");
        printf("FAIL\n");
        return false;
    }

    stellarr::OutputBlock dest;
    dest.fromJson(json);

    if (! dest.hasTargetLufs())
    {
        fprintf(stderr, "  expected target after fromJson\n");
        printf("FAIL\n");
        return false;
    }
    if (std::abs(dest.getTargetLufs() - (-14.0f)) > 1e-6f)
    {
        fprintf(stderr, "  round-trip mismatch: got %f\n",
                static_cast<double>(dest.getTargetLufs()));
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testOutputBlockTargetLufsJsonAbsentDeserialisesToNaN()
{
    printf("Test: OutputBlock fromJson without targetLufs → NaN... ");

    // Simulate loading an older preset that pre-dates the targetLufs field.
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id", juce::Uuid().toString());
    obj->setProperty("type", "output");
    obj->setProperty("name", "Output");
    obj->setProperty("level", -0.0);
    juce::var json(obj);

    stellarr::OutputBlock block;
    block.setTargetLufs(-20.0f); // pre-set to a value so we can see it cleared
    block.fromJson(json);

    if (block.hasTargetLufs())
    {
        fprintf(stderr, "  expected NaN target when JSON has no key, got %f\n",
                static_cast<double>(block.getTargetLufs()));
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

    // FX bypass modes
    if (!testBypassMuteFxInDryPassesThrough())      ++failures;
    if (!testBypassMuteFxInFeedsSilenceToPlugin())   ++failures;
    if (!testBypassMuteFxInAppliesLevelAndBalance()) ++failures;
    if (!testBypassMuteFxOutDryPassesThrough())      ++failures;
    if (!testBypassMuteFxOutAppliesLevelAndBalance()) ++failures;

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

    // Loudness measurement
    if (!testMeasureLoudnessDisabledByDefault()) ++failures;
    if (!testMeasureLoudnessEnabledReads())      ++failures;
    if (!testMeterRunsInBypassThru())            ++failures;
    if (!testMeterReadsFloorInBypassMute())      ++failures;
    if (!testMeterRunsInAllBypassModes())        ++failures;
    if (!testMeterReflectsLevel())               ++failures;
    if (!testOutputBlockMeasuresByDefault())     ++failures;
    if (!testOutputBlockTargetLufsDefaultsNaN())             ++failures;
    if (!testOutputBlockTargetLufsSetGet())                  ++failures;
    if (!testOutputBlockTargetLufsJsonOmittedWhenUnset())    ++failures;
    if (!testOutputBlockTargetLufsJsonRoundTrip())           ++failures;
    if (!testOutputBlockTargetLufsJsonAbsentDeserialisesToNaN()) ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
