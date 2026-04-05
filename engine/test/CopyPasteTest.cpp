#include "TestUtils.h"
#include "../StellarrBridge.h"
#include "../blocks/PluginBlock.h"

// Friend access wrapper for private bridge methods
class CopyPasteTestAccess
{
public:
    static void addBlock(StellarrBridge& b, const juce::var& j) { b.handleAddBlock(j); }
    static void copyBlock(StellarrBridge& b, const juce::var& j) { b.handleCopyBlock(j); }
    static void pasteBlock(StellarrBridge& b, const juce::var& j) { b.handlePasteBlock(j); }
    static const juce::var& getClipboard(StellarrBridge& b) { return b.clipboardJson; }
    static const auto& getBlockNodeMap(StellarrBridge& b) { return b.blockNodeMap; }
    static const auto& getBlockPositions(StellarrBridge& b) { return b.blockPositions; }
};

// Helper: set up a bridge with a processor and add a plugin block at a position
struct TestSetup
{
    StellarrProcessor proc;
    StellarrBridge bridge;
    juce::String blockId;

    TestSetup()
    {
        proc.prepareToPlay(kSampleRate, kBlockSize);
        bridge.setProcessor(&proc);

        // Add a plugin block via the bridge's handleAddBlock
        auto json = juce::JSON::parse(R"({"type":"plugin","col":3,"row":2})");
        CopyPasteTestAccess::addBlock(bridge, json);

        // Find the block that was added
        auto& map = CopyPasteTestAccess::getBlockNodeMap(bridge);
        if (!map.empty())
            blockId = map.begin()->first;
    }

    ~TestSetup()
    {
        proc.releaseResources();
    }
};

// -- Copy tests ---------------------------------------------------------------

static bool testCopyStoresClipboard()
{
    printf("Test: copy block stores clipboard data... ");

    TestSetup setup;
    if (setup.blockId.isEmpty())
    {
        printf("FAIL (no block created)\n");
        return false;
    }

    auto json = juce::JSON::parse("{\"blockId\":\"" + setup.blockId + "\"}");
    CopyPasteTestAccess::copyBlock(setup.bridge, json);

    auto& clipboard = CopyPasteTestAccess::getClipboard(setup.bridge);
    if (!clipboard.getDynamicObject())
    {
        printf("FAIL (clipboard empty after copy)\n");
        return false;
    }

    auto clipType = clipboard.getDynamicObject()->getProperty("type").toString();
    if (clipType != "plugin")
    {
        printf("FAIL (expected type 'plugin', got '%s')\n", clipType.toRawUTF8());
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testCopyInvalidBlockIsNoop()
{
    printf("Test: copy non-existent block is a no-op... ");

    TestSetup setup;

    auto json = juce::JSON::parse(R"({"blockId":"non-existent-id"})");
    CopyPasteTestAccess::copyBlock(setup.bridge, json);

    auto& clipboard = CopyPasteTestAccess::getClipboard(setup.bridge);
    if (clipboard.getDynamicObject())
    {
        printf("FAIL (clipboard should be empty)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

// -- Paste tests --------------------------------------------------------------

static bool testPasteCreatesNewBlock()
{
    printf("Test: paste creates a new block... ");

    TestSetup setup;
    if (setup.blockId.isEmpty()) { printf("FAIL (no block)\n"); return false; }

    // Copy
    auto copyJson = juce::JSON::parse("{\"blockId\":\"" + setup.blockId + "\"}");
    CopyPasteTestAccess::copyBlock(setup.bridge, copyJson);

    auto& mapBefore = CopyPasteTestAccess::getBlockNodeMap(setup.bridge);
    int countBefore = static_cast<int>(mapBefore.size());

    // Paste
    auto pasteJson = juce::JSON::parse(R"({"col":5,"row":2})");
    CopyPasteTestAccess::pasteBlock(setup.bridge, pasteJson);

    auto& mapAfter = CopyPasteTestAccess::getBlockNodeMap(setup.bridge);
    int countAfter = static_cast<int>(mapAfter.size());

    if (countAfter != countBefore + 1)
    {
        printf("FAIL (expected %d blocks, got %d)\n", countBefore + 1, countAfter);
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testPasteGetsNewId()
{
    printf("Test: pasted block gets a new unique ID... ");

    TestSetup setup;
    if (setup.blockId.isEmpty()) { printf("FAIL (no block)\n"); return false; }

    auto copyJson = juce::JSON::parse("{\"blockId\":\"" + setup.blockId + "\"}");
    CopyPasteTestAccess::copyBlock(setup.bridge, copyJson);

    auto pasteJson = juce::JSON::parse(R"({"col":5,"row":2})");
    CopyPasteTestAccess::pasteBlock(setup.bridge, pasteJson);

    // Find the new block (not the original)
    auto& map = CopyPasteTestAccess::getBlockNodeMap(setup.bridge);
    bool foundOriginal = false;
    bool foundNew = false;
    for (auto& [id, nodeId] : map)
    {
        if (id == setup.blockId)
            foundOriginal = true;
        else
            foundNew = true;
    }

    if (!foundOriginal || !foundNew)
    {
        printf("FAIL (expected both original and new block)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testPastePreservesProperties()
{
    printf("Test: pasted block preserves properties... ");

    TestSetup setup;
    if (setup.blockId.isEmpty()) { printf("FAIL (no block)\n"); return false; }

    // Set properties on the original block
    auto nodeIt = CopyPasteTestAccess::getBlockNodeMap(setup.bridge).find(setup.blockId);
    auto* node = setup.proc.getGraph().getNodeForId(nodeIt->second);
    auto* block = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
    block->setDisplayName("My Amp");
    block->setBlockColor("#ff0000");
    block->setMix(0.7f);
    block->setBalance(-0.3f);
    block->setLevelDb(-6.0f);
    block->setBypassed(true);
    block->setBypassMode(stellarr::BypassMode::muteOut);

    // Copy
    auto copyJson = juce::JSON::parse("{\"blockId\":\"" + setup.blockId + "\"}");
    CopyPasteTestAccess::copyBlock(setup.bridge, copyJson);

    // Paste
    auto pasteJson = juce::JSON::parse(R"({"col":7,"row":3})");
    CopyPasteTestAccess::pasteBlock(setup.bridge, pasteJson);

    // Find the pasted block
    stellarr::PluginBlock* pasted = nullptr;
    for (auto& [id, nid] : CopyPasteTestAccess::getBlockNodeMap(setup.bridge))
    {
        if (id == setup.blockId) continue;
        auto* n = setup.proc.getGraph().getNodeForId(nid);
        if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(n->getProcessor()))
            pasted = pb;
    }

    if (pasted == nullptr)
    {
        printf("FAIL (pasted block not found)\n");
        return false;
    }

    bool ok = true;
    if (pasted->getDisplayName() != "My Amp")
    { fprintf(stderr, "  displayName mismatch: '%s'\n", pasted->getDisplayName().toRawUTF8()); ok = false; }
    if (pasted->getBlockColor() != "#ff0000")
    { fprintf(stderr, "  blockColor mismatch\n"); ok = false; }
    if (std::abs(pasted->getMix() - 0.7f) > 0.01f)
    { fprintf(stderr, "  mix mismatch: %f\n", static_cast<double>(pasted->getMix())); ok = false; }
    if (std::abs(pasted->getBalance() - (-0.3f)) > 0.01f)
    { fprintf(stderr, "  balance mismatch\n"); ok = false; }
    if (std::abs(pasted->getLevelDb() - (-6.0f)) > 0.5f)
    { fprintf(stderr, "  level mismatch\n"); ok = false; }
    if (!pasted->isBypassed())
    { fprintf(stderr, "  bypassed not preserved\n"); ok = false; }
    if (pasted->getBypassMode() != stellarr::BypassMode::muteOut)
    { fprintf(stderr, "  bypassMode not preserved\n"); ok = false; }

    printf("%s\n", ok ? "PASS" : "FAIL");
    return ok;
}

static bool testPastePosition()
{
    printf("Test: pasted block placed at specified position... ");

    TestSetup setup;
    if (setup.blockId.isEmpty()) { printf("FAIL (no block)\n"); return false; }

    auto copyJson = juce::JSON::parse("{\"blockId\":\"" + setup.blockId + "\"}");
    CopyPasteTestAccess::copyBlock(setup.bridge, copyJson);

    auto pasteJson = juce::JSON::parse(R"({"col":8,"row":4})");
    CopyPasteTestAccess::pasteBlock(setup.bridge, pasteJson);

    // Find the pasted block's position
    auto& positions = CopyPasteTestAccess::getBlockPositions(setup.bridge);
    bool found = false;
    for (auto& [id, pos] : positions)
    {
        if (id == setup.blockId) continue;
        if (pos.first == 8 && pos.second == 4)
            found = true;
    }

    if (!found)
    {
        printf("FAIL (pasted block not at col=8, row=4)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testPasteWithoutCopyIsNoop()
{
    printf("Test: paste without prior copy is a no-op... ");

    TestSetup setup;

    auto& mapBefore = CopyPasteTestAccess::getBlockNodeMap(setup.bridge);
    int countBefore = static_cast<int>(mapBefore.size());

    auto pasteJson = juce::JSON::parse(R"({"col":5,"row":2})");
    CopyPasteTestAccess::pasteBlock(setup.bridge, pasteJson);

    auto& mapAfter = CopyPasteTestAccess::getBlockNodeMap(setup.bridge);
    int countAfter = static_cast<int>(mapAfter.size());

    if (countAfter != countBefore)
    {
        printf("FAIL (block count changed: %d → %d)\n", countBefore, countAfter);
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testMultiplePastesGetUniqueIds()
{
    printf("Test: multiple pastes from same clipboard get unique IDs... ");

    TestSetup setup;
    if (setup.blockId.isEmpty()) { printf("FAIL (no block)\n"); return false; }

    auto copyJson = juce::JSON::parse("{\"blockId\":\"" + setup.blockId + "\"}");
    CopyPasteTestAccess::copyBlock(setup.bridge, copyJson);

    auto paste1 = juce::JSON::parse(R"({"col":5,"row":0})");
    CopyPasteTestAccess::pasteBlock(setup.bridge, paste1);

    auto paste2 = juce::JSON::parse(R"({"col":6,"row":0})");
    CopyPasteTestAccess::pasteBlock(setup.bridge, paste2);

    auto& map = CopyPasteTestAccess::getBlockNodeMap(setup.bridge);
    if (static_cast<int>(map.size()) != 3)
    {
        printf("FAIL (expected 3 blocks, got %d)\n", static_cast<int>(map.size()));
        return false;
    }

    // All IDs should be unique (map keys are unique by definition)
    std::set<juce::String> ids;
    for (auto& [id, _] : map)
        ids.insert(id);

    if (static_cast<int>(ids.size()) != 3)
    {
        printf("FAIL (IDs not unique)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    // Copy
    if (!testCopyStoresClipboard())       ++failures;
    if (!testCopyInvalidBlockIsNoop())    ++failures;

    // Paste
    if (!testPasteCreatesNewBlock())      ++failures;
    if (!testPasteGetsNewId())            ++failures;
    if (!testPastePreservesProperties())  ++failures;
    if (!testPastePosition())             ++failures;
    if (!testPasteWithoutCopyIsNoop())    ++failures;
    if (!testMultiplePastesGetUniqueIds()) ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
