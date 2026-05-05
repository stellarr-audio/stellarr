#include "TestUtils.h"
#include "../StellarrBridge.h"

// Friend access wrapper for private bridge methods
class PresetFileTestAccess
{
public:
    static void getPresetList(StellarrBridge& b) { b.preset->handleGetPresetList(); }
    static void renamePreset(StellarrBridge& b, const juce::var& j) { b.preset->handleRenamePreset(j); }
    static void deletePreset(StellarrBridge& b, const juce::var& j) { b.preset->handleDeletePreset(j); }
    static void loadPresetByIndex(StellarrBridge& b, const juce::var& j) { b.preset->handleLoadPresetByIndex(j); }
};

// handleLoadPresetByIndex now schedules its restore via juce::AsyncUpdater.
// Pump the message loop until the bookkeeping callback either updates
// currentPresetIndex or the timeout expires.
static void waitForPresetLoad(StellarrBridge& bridge, int expectedIndex, int timeoutMs = 5000)
{
    auto deadline = juce::Time::getMillisecondCounter() + static_cast<juce::uint32>(timeoutMs);
    while (bridge.getCurrentPresetIndex() != expectedIndex
           && juce::Time::getMillisecondCounter() < deadline)
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }
}

// Helper: create a temp preset directory with N .stellarr files
static juce::File createTempPresetDir(int count)
{
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile("stellarr_test_presets_" + juce::String(juce::Random::getSystemRandom().nextInt()));
    dir.createDirectory();

    // Use a minimal-but-restorable session shape (input + output, no
    // connections) so restoreSession succeeds and the bookkeeping path runs.
    // A bare {"blocks":[]} parses but loadPresetByIndex now gates state on
    // restoreSession returning true — and that requires a processor anyway.
    static constexpr const char* kMinimalSession = R"({
        "version": 1,
        "blocks": [
            {"id":"in1","type":"input","name":"Input","col":0,"row":2},
            {"id":"out1","type":"output","name":"Output","col":11,"row":2}
        ],
        "connections": [{"sourceId":"in1","destId":"out1"}],
        "scenes": [],
        "activeSceneIndex": -1
    })";

    for (int i = 0; i < count; ++i)
    {
        char letter = static_cast<char>('A' + i);
        auto name = "Preset_" + juce::String(&letter, 1);
        auto file = dir.getChildFile(name + ".stellarr");
        file.replaceWithText(kMinimalSession);
    }

    return dir;
}

static void cleanupDir(const juce::File& dir)
{
    dir.deleteRecursively();
}

// -- Rename tests -------------------------------------------------------------

static bool testRenamePreset()
{
    printf("Test: rename preset renames file on disk... ");

    auto dir = createTempPresetDir(3); // A, B, C
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    // Files should be sorted: Preset_A, Preset_B, Preset_C
    if (bridge.getPresetFiles().size() != 3)
    {
        printf("FAIL (expected 3 files, got %d)\n", bridge.getPresetFiles().size());
        cleanupDir(dir);
        return false;
    }

    // Rename Preset_B → Preset_Renamed
    auto json = juce::JSON::parse(R"({"index":1,"name":"Preset_Renamed"})");
    PresetFileTestAccess::renamePreset(bridge,json);

    if (!dir.getChildFile("Preset_Renamed.stellarr").existsAsFile())
    {
        printf("FAIL (renamed file not found)\n");
        cleanupDir(dir);
        return false;
    }

    if (dir.getChildFile("Preset_B.stellarr").existsAsFile())
    {
        printf("FAIL (old file still exists)\n");
        cleanupDir(dir);
        return false;
    }

    if (bridge.getPresetFiles().size() != 3)
    {
        printf("FAIL (expected 3 files after rename, got %d)\n", bridge.getPresetFiles().size());
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

static bool testRenameActivePreset()
{
    printf("Test: rename active preset updates tracking... ");

    auto dir = createTempPresetDir(2); // A, B
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    // Simulate loading preset B (index 1)
    auto loadJson = juce::JSON::parse(R"({"index":1})");
    PresetFileTestAccess::loadPresetByIndex(bridge,loadJson);
    waitForPresetLoad(bridge, 1);

    if (bridge.getCurrentPresetIndex() != 1)
    {
        printf("FAIL (precondition: expected index 1)\n");
        cleanupDir(dir);
        return false;
    }

    // Rename active preset B → Preset_Z (sorts to end)
    auto renameJson = juce::JSON::parse(R"({"index":1,"name":"Preset_Z"})");
    PresetFileTestAccess::renamePreset(bridge,renameJson);

    // After rename + re-sort: Preset_A (0), Preset_Z (1)
    // Active should now be at index 1 (Preset_Z)
    if (bridge.getCurrentPresetIndex() != 1)
    {
        printf("FAIL (expected index 1, got %d)\n", bridge.getCurrentPresetIndex());
        cleanupDir(dir);
        return false;
    }

    if (bridge.getLastPresetFile().getFileName() != "Preset_Z.stellarr")
    {
        printf("FAIL (lastPresetFile not updated: %s)\n",
               bridge.getLastPresetFile().getFileName().toRawUTF8());
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

static bool testRenameToExistingName()
{
    printf("Test: rename to existing name is rejected... ");

    auto dir = createTempPresetDir(2); // A, B
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    // Try renaming A → Preset_B (already exists)
    auto json = juce::JSON::parse(R"({"index":0,"name":"Preset_B"})");
    PresetFileTestAccess::renamePreset(bridge,json);

    // Both files should still exist
    if (!dir.getChildFile("Preset_A.stellarr").existsAsFile() ||
        !dir.getChildFile("Preset_B.stellarr").existsAsFile())
    {
        printf("FAIL (files were modified)\n");
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

static bool testRenameEmptyName()
{
    printf("Test: rename with empty name is rejected... ");

    auto dir = createTempPresetDir(1);
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    auto json = juce::JSON::parse(R"({"index":0,"name":"  "})");
    PresetFileTestAccess::renamePreset(bridge,json);

    // Original file should still exist
    if (!dir.getChildFile("Preset_A.stellarr").existsAsFile())
    {
        printf("FAIL (file was modified)\n");
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

// -- Delete tests -------------------------------------------------------------

static bool testDeletePreset()
{
    printf("Test: delete preset removes file from disk... ");

    auto dir = createTempPresetDir(3); // A, B, C
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    auto json = juce::JSON::parse(R"({"index":1})");
    PresetFileTestAccess::deletePreset(bridge,json);

    if (dir.getChildFile("Preset_B.stellarr").existsAsFile())
    {
        printf("FAIL (file still exists)\n");
        cleanupDir(dir);
        return false;
    }

    if (bridge.getPresetFiles().size() != 2)
    {
        printf("FAIL (expected 2 files, got %d)\n", bridge.getPresetFiles().size());
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

static bool testDeleteActivePreset()
{
    printf("Test: delete active preset clears active state... ");

    auto dir = createTempPresetDir(3);
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    // Load preset B (index 1)
    auto loadJson = juce::JSON::parse(R"({"index":1})");
    PresetFileTestAccess::loadPresetByIndex(bridge,loadJson);
    waitForPresetLoad(bridge, 1);

    // Delete it
    auto deleteJson = juce::JSON::parse(R"({"index":1})");
    PresetFileTestAccess::deletePreset(bridge,deleteJson);

    if (bridge.getCurrentPresetIndex() != -1)
    {
        printf("FAIL (expected index -1, got %d)\n", bridge.getCurrentPresetIndex());
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

static bool testDeleteBeforeActive()
{
    printf("Test: delete before active shifts index down... ");

    auto dir = createTempPresetDir(3); // A(0), B(1), C(2)
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    // Load preset C (index 2)
    auto loadJson = juce::JSON::parse(R"({"index":2})");
    PresetFileTestAccess::loadPresetByIndex(bridge,loadJson);
    waitForPresetLoad(bridge, 2);

    // Delete preset A (index 0) — active should shift from 2 to 1
    auto deleteJson = juce::JSON::parse(R"({"index":0})");
    PresetFileTestAccess::deletePreset(bridge,deleteJson);

    if (bridge.getCurrentPresetIndex() != 1)
    {
        printf("FAIL (expected index 1, got %d)\n", bridge.getCurrentPresetIndex());
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

static bool testDeleteAfterActive()
{
    printf("Test: delete after active leaves index unchanged... ");

    auto dir = createTempPresetDir(3); // A(0), B(1), C(2)
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    // Load preset A (index 0)
    auto loadJson = juce::JSON::parse(R"({"index":0})");
    PresetFileTestAccess::loadPresetByIndex(bridge,loadJson);
    waitForPresetLoad(bridge, 0);

    // Delete preset C (index 2) — active should stay at 0
    auto deleteJson = juce::JSON::parse(R"({"index":2})");
    PresetFileTestAccess::deletePreset(bridge,deleteJson);

    if (bridge.getCurrentPresetIndex() != 0)
    {
        printf("FAIL (expected index 0, got %d)\n", bridge.getCurrentPresetIndex());
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

static bool testDeleteInvalidIndex()
{
    printf("Test: delete with invalid index is a no-op... ");

    auto dir = createTempPresetDir(2);
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);
    StellarrBridge bridge;
    bridge.setProcessor(&proc);
    bridge.setPresetDirectory(dir);
    PresetFileTestAccess::getPresetList(bridge);

    auto json = juce::JSON::parse(R"({"index":5})");
    PresetFileTestAccess::deletePreset(bridge,json);

    if (bridge.getPresetFiles().size() != 2)
    {
        printf("FAIL (files were modified)\n");
        cleanupDir(dir);
        return false;
    }

    cleanupDir(dir);
    printf("PASS\n");
    return true;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    int failures = 0;

    // Rename
    if (!testRenamePreset())          ++failures;
    if (!testRenameActivePreset())    ++failures;
    if (!testRenameToExistingName())  ++failures;
    if (!testRenameEmptyName())       ++failures;

    // Delete
    if (!testDeletePreset())          ++failures;
    if (!testDeleteActivePreset())    ++failures;
    if (!testDeleteBeforeActive())    ++failures;
    if (!testDeleteAfterActive())     ++failures;
    if (!testDeleteInvalidIndex())    ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
