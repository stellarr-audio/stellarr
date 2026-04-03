#include "TestUtils.h"
#include "MidiMapper.h"

// -- CC Interception ----------------------------------------------------------

static bool testMappedCcRemoved()
{
    printf("Test: mapped CC removed from buffer, unmapped passes through... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 7;
    m.target = MidiMapper::Target::blockMix;
    m.blockId = "block-1";
    mapper.addMapping(m);

    bool callbackFired = false;
    mapper.onBlockMix = [&](const juce::String&, float) { callbackFired = true; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 7, 100), 0);   // mapped
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 11, 50), 10);  // unmapped

    mapper.processMidi(buf);

    if (!callbackFired)
    {
        fprintf(stderr, "  callback not fired for mapped CC\n");
        printf("FAIL\n");
        return false;
    }

    int count = 0;
    for (const auto metadata : buf)
    {
        auto msg = metadata.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 7)
        {
            fprintf(stderr, "  CC 7 should have been consumed\n");
            printf("FAIL\n");
            return false;
        }
        if (msg.isController() && msg.getControllerNumber() == 11)
            ++count;
    }

    if (count != 1)
    {
        fprintf(stderr, "  CC 11 should pass through, found %d\n", count);
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testProgramChangePassthrough()
{
    printf("Test: Program Change passes through without mapping... ");

    MidiMapper mapper;

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::programChange(1, 5), 0);

    mapper.processMidi(buf);

    int count = 0;
    for (const auto metadata : buf)
        if (metadata.getMessage().isProgramChange())
            ++count;

    if (count != 1)
    {
        fprintf(stderr, "  PC should pass through, found %d\n", count);
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

// -- Target Callbacks ---------------------------------------------------------

static bool testBlockBypassCallback()
{
    printf("Test: blockBypass callback with threshold... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 20;
    m.target = MidiMapper::Target::blockBypass;
    m.blockId = "block-bp";
    mapper.addMapping(m);

    bool lastState = false;
    mapper.onBlockBypass = [&](const juce::String&, bool state) { lastState = state; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 20, 0), 0);
    mapper.processMidi(buf);
    if (lastState != false)
    {
        fprintf(stderr, "  value 0 should be off\n");
        printf("FAIL\n");
        return false;
    }

    buf.clear();
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 20, 100), 0);
    mapper.processMidi(buf);
    if (lastState != true)
    {
        fprintf(stderr, "  value 100 should be on\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testBlockMixScaling()
{
    printf("Test: blockMix CC scaling 0→0.0, 127→1.0... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 7;
    m.target = MidiMapper::Target::blockMix;
    m.blockId = "b";
    mapper.addMapping(m);

    float lastValue = -1;
    mapper.onBlockMix = [&](const juce::String&, float v) { lastValue = v; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 7, 0), 0);
    mapper.processMidi(buf);
    if (std::abs(lastValue) > 0.01f) { printf("FAIL (0)\n"); return false; }

    buf.clear();
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 7, 127), 0);
    mapper.processMidi(buf);
    if (std::abs(lastValue - 1.0f) > 0.01f) { printf("FAIL (127)\n"); return false; }

    printf("PASS\n");
    return true;
}

static bool testBlockBalanceScaling()
{
    printf("Test: blockBalance bipolar scaling... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 10;
    m.target = MidiMapper::Target::blockBalance;
    m.blockId = "b";
    mapper.addMapping(m);

    float lastValue = 0;
    mapper.onBlockBalance = [&](const juce::String&, float v) { lastValue = v; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 10, 0), 0);
    mapper.processMidi(buf);
    if (std::abs(lastValue - (-1.0f)) > 0.05f) { printf("FAIL (0→-1)\n"); return false; }

    buf.clear();
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 10, 64), 0);
    mapper.processMidi(buf);
    if (std::abs(lastValue) > 0.05f) { printf("FAIL (64→0)\n"); return false; }

    buf.clear();
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 10, 127), 0);
    mapper.processMidi(buf);
    if (std::abs(lastValue - 1.0f) > 0.05f) { printf("FAIL (127→1)\n"); return false; }

    printf("PASS\n");
    return true;
}

static bool testBlockLevelScaling()
{
    printf("Test: blockLevel dB scaling 0→-60, 127→12... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 17;
    m.target = MidiMapper::Target::blockLevel;
    m.blockId = "b";
    mapper.addMapping(m);

    float lastValue = 0;
    mapper.onBlockLevel = [&](const juce::String&, float v) { lastValue = v; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 17, 0), 0);
    mapper.processMidi(buf);
    if (std::abs(lastValue - (-60.0f)) > 0.5f) { printf("FAIL (0→-60)\n"); return false; }

    buf.clear();
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 17, 127), 0);
    mapper.processMidi(buf);
    if (std::abs(lastValue - 12.0f) > 0.5f) { printf("FAIL (127→12)\n"); return false; }

    printf("PASS\n");
    return true;
}

static bool testSceneSwitchCallback()
{
    printf("Test: sceneSwitch uses CC value as index... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 8;
    m.target = MidiMapper::Target::sceneSwitch;
    mapper.addMapping(m);

    int lastIndex = -1;
    mapper.onSceneSwitch = [&](int idx) { lastIndex = idx; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 8, 3), 0);
    mapper.processMidi(buf);

    if (lastIndex != 3) { printf("FAIL\n"); return false; }

    printf("PASS\n");
    return true;
}

static bool testPresetChangeFromPC()
{
    printf("Test: presetChange from Program Change... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = -1; // PC mode
    m.target = MidiMapper::Target::presetChange;
    mapper.addMapping(m);

    int lastPC = -1;
    mapper.onPresetChange = [&](int idx) { lastPC = idx; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::programChange(1, 7), 0);
    mapper.processMidi(buf);

    if (lastPC != 7) { printf("FAIL\n"); return false; }

    printf("PASS\n");
    return true;
}

// -- MIDI Learn ---------------------------------------------------------------

static bool testMidiLearn()
{
    printf("Test: MIDI Learn creates mapping from first CC... ");

    MidiMapper mapper;
    mapper.startLearn(MidiMapper::Target::blockMix, "block-learn");

    if (!mapper.isLearning()) { printf("FAIL (not learning)\n"); return false; }

    int learnCh = -1, learnCc = -1;
    mapper.onLearnComplete = [&](int ch, int cc) { learnCh = ch; learnCc = cc; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(4, 22, 64), 0); // channel 4 (1-indexed)
    mapper.processMidi(buf);

    if (mapper.isLearning()) { fprintf(stderr, "  still learning\n"); printf("FAIL\n"); return false; }
    if (mapper.getNumMappings() != 1) { fprintf(stderr, "  no mapping created\n"); printf("FAIL\n"); return false; }
    if (learnCh != 3 || learnCc != 22) { fprintf(stderr, "  wrong ch/cc\n"); printf("FAIL\n"); return false; }

    auto& created = mapper.getMapping(0);
    if (created.target != MidiMapper::Target::blockMix || created.blockId != "block-learn")
    {
        fprintf(stderr, "  wrong target/blockId\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testCancelLearn()
{
    printf("Test: cancelLearn stops without creating mapping... ");

    MidiMapper mapper;
    mapper.startLearn(MidiMapper::Target::blockLevel, "x");
    mapper.cancelLearn();

    if (mapper.isLearning()) { printf("FAIL (still learning)\n"); return false; }

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 50, 64), 0);
    mapper.processMidi(buf);

    if (mapper.getNumMappings() != 0) { printf("FAIL (mapping created)\n"); return false; }

    printf("PASS\n");
    return true;
}

// -- Channel Filtering --------------------------------------------------------

static bool testChannelFiltering()
{
    printf("Test: channel-specific mapping ignores other channels... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = 5; // only channel 5 (0-indexed)
    m.ccNumber = 11;
    m.target = MidiMapper::Target::blockMix;
    m.blockId = "b";
    mapper.addMapping(m);

    int callCount = 0;
    mapper.onBlockMix = [&](const juce::String&, float) { ++callCount; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(5, 11, 64), 0);  // channel 4 (0-indexed) — wrong
    buf.addEvent(juce::MidiMessage::controllerEvent(6, 11, 64), 10); // channel 5 (0-indexed) — match

    mapper.processMidi(buf);

    if (callCount != 1)
    {
        fprintf(stderr, "  expected 1 callback, got %d\n", callCount);
        printf("FAIL\n");
        return false;
    }

    // Channel 4's CC 11 should still be in buffer (not consumed)
    int remaining = 0;
    for (const auto metadata : buf)
        if (metadata.getMessage().isController())
            ++remaining;

    if (remaining != 1)
    {
        fprintf(stderr, "  wrong channel CC should remain, got %d\n", remaining);
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testAnyChannel()
{
    printf("Test: channel -1 catches all channels... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 7;
    m.target = MidiMapper::Target::blockMix;
    m.blockId = "b";
    mapper.addMapping(m);

    int callCount = 0;
    mapper.onBlockMix = [&](const juce::String&, float) { ++callCount; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 7, 64), 0);
    buf.addEvent(juce::MidiMessage::controllerEvent(9, 7, 64), 10);
    buf.addEvent(juce::MidiMessage::controllerEvent(16, 7, 64), 20);
    mapper.processMidi(buf);

    if (callCount != 3) { fprintf(stderr, "  expected 3, got %d\n", callCount); printf("FAIL\n"); return false; }

    printf("PASS\n");
    return true;
}

// -- Serialization ------------------------------------------------------------

static bool testSerialisationRoundtrip()
{
    printf("Test: MIDI mapping serialisation roundtrip... ");

    MidiMapper original;

    MidiMapper::Mapping m1;
    m1.channel = 3;
    m1.ccNumber = 7;
    m1.target = MidiMapper::Target::blockMix;
    m1.blockId = "amp";
    original.addMapping(m1);

    MidiMapper::Mapping m2;
    m2.channel = -1;
    m2.ccNumber = -1;
    m2.target = MidiMapper::Target::presetChange;
    original.addMapping(m2);

    auto json = original.toJson();

    MidiMapper restored;
    restored.fromJson(json);

    if (restored.getNumMappings() != 2) { printf("FAIL (count)\n"); return false; }

    auto& r1 = restored.getMapping(0);
    if (r1.channel != 3 || r1.ccNumber != 7 || r1.target != MidiMapper::Target::blockMix || r1.blockId != "amp")
    { printf("FAIL (m1)\n"); return false; }

    auto& r2 = restored.getMapping(1);
    if (r2.channel != -1 || r2.ccNumber != -1 || r2.target != MidiMapper::Target::presetChange)
    { printf("FAIL (m2)\n"); return false; }

    printf("PASS\n");
    return true;
}

static bool testFromJsonClearsOld()
{
    printf("Test: fromJson clears old mappings... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 1;
    m.target = MidiMapper::Target::blockMix;
    mapper.addMapping(m);
    mapper.addMapping(m);

    // Load single mapping
    MidiMapper single;
    MidiMapper::Mapping m2;
    m2.channel = 0;
    m2.ccNumber = 99;
    m2.target = MidiMapper::Target::blockLevel;
    single.addMapping(m2);

    mapper.fromJson(single.toJson());

    if (mapper.getNumMappings() != 1) { printf("FAIL\n"); return false; }
    if (mapper.getMapping(0).ccNumber != 99) { printf("FAIL (wrong cc)\n"); return false; }

    printf("PASS\n");
    return true;
}

static bool testPresetGlobalSplit()
{
    printf("Test: preset/global mapping split serialisation... ");

    MidiMapper mapper;

    MidiMapper::Mapping preset;
    preset.channel = -1;
    preset.ccNumber = 7;
    preset.target = MidiMapper::Target::blockMix;
    preset.blockId = "amp";
    mapper.addMapping(preset);

    MidiMapper::Mapping global;
    global.channel = -1;
    global.ccNumber = -1;
    global.target = MidiMapper::Target::presetChange;
    mapper.addMapping(global);

    MidiMapper::Mapping globalTuner;
    globalTuner.channel = -1;
    globalTuner.ccNumber = 64;
    globalTuner.target = MidiMapper::Target::tunerToggle;
    mapper.addMapping(globalTuner);

    auto presetJson = mapper.presetMappingsToJson();
    auto globalJson = mapper.globalMappingsToJson();

    if (presetJson.getArray()->size() != 1) { printf("FAIL (preset count)\n"); return false; }
    if (globalJson.getArray()->size() != 2) { printf("FAIL (global count)\n"); return false; }

    // Load only preset mappings — should keep globals
    MidiMapper restored;
    restored.loadGlobalMappings(globalJson);
    restored.loadPresetMappings(presetJson);

    if (restored.getNumMappings() != 3) { fprintf(stderr, "  expected 3, got %d\n", restored.getNumMappings()); printf("FAIL\n"); return false; }

    printf("PASS\n");
    return true;
}

// -- Edge Cases ---------------------------------------------------------------

static bool testNoCallbackNoCrash()
{
    printf("Test: no callback set doesn't crash... ");

    MidiMapper mapper;
    MidiMapper::Mapping m;
    m.channel = -1;
    m.ccNumber = 7;
    m.target = MidiMapper::Target::blockMix;
    m.blockId = "b";
    mapper.addMapping(m);
    // Don't set onBlockMix callback

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 7, 64), 0);
    mapper.processMidi(buf); // should not crash

    printf("PASS\n");
    return true;
}

static bool testEmptyBufferNoCrash()
{
    printf("Test: empty buffer doesn't crash... ");

    MidiMapper mapper;
    juce::MidiBuffer buf;
    mapper.processMidi(buf);

    printf("PASS\n");
    return true;
}

static bool testActivityCallbackOnUnmapped()
{
    printf("Test: activity callback fires on unmapped CC... ");

    MidiMapper mapper;
    int actCh = -1, actCc = -1, actVal = -1;
    mapper.onMidiActivity = [&](int ch, int cc, int val) { actCh = ch; actCc = cc; actVal = val; };

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(3, 100, 42), 0);
    mapper.processMidi(buf);

    if (actCh != 2 || actCc != 100 || actVal != 42)
    {
        fprintf(stderr, "  activity not reported\n");
        printf("FAIL\n");
        return false;
    }

    // CC 100 should still be in buffer (unmapped)
    int count = 0;
    for (const auto metadata : buf)
        if (metadata.getMessage().isController())
            ++count;

    if (count != 1) { printf("FAIL (consumed)\n"); return false; }

    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    // CC Interception
    if (!testMappedCcRemoved())         ++failures;
    if (!testProgramChangePassthrough()) ++failures;

    // Target Callbacks
    if (!testBlockBypassCallback())     ++failures;
    if (!testBlockMixScaling())         ++failures;
    if (!testBlockBalanceScaling())     ++failures;
    if (!testBlockLevelScaling())       ++failures;
    if (!testSceneSwitchCallback())     ++failures;
    if (!testPresetChangeFromPC())      ++failures;

    // MIDI Learn
    if (!testMidiLearn())               ++failures;
    if (!testCancelLearn())             ++failures;

    // Channel Filtering
    if (!testChannelFiltering())        ++failures;
    if (!testAnyChannel())              ++failures;

    // Serialization
    if (!testSerialisationRoundtrip())  ++failures;
    if (!testFromJsonClearsOld())       ++failures;
    if (!testPresetGlobalSplit())       ++failures;

    // Edge Cases
    if (!testNoCallbackNoCrash())       ++failures;
    if (!testEmptyBufferNoCrash())      ++failures;
    if (!testActivityCallbackOnUnmapped()) ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
