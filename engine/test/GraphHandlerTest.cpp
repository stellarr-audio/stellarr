#include <cstdio>
#include <juce_core/juce_core.h>
#include <map>
#include <utility>
#include "../StellarrProcessor.h"
#include "../bridge/BridgeTypes.h"
#include "../bridge/GraphHandler.h"
#include "support/MockBridgeEmitter.h"

// Per-handler isolation tests for GraphHandler. A StellarrProcessor is
// constructed directly; the BlockNodeMap, blockPositions and clipboard
// are owned by the test; cross-handler callbacks (markBlockDirty,
// emitMidiMappings, sendGraphState) are no-op lambdas. No
// StellarrBridge is involved.

namespace
{
    struct GraphHandlerFixture
    {
        StellarrProcessor processor;
        stellarr::bridge::BlockNodeMap blockNodeMap;
        std::map<juce::String, std::pair<int, int>> blockPositions;
        juce::var clipboardJson;

        GraphHandlerFixture()
        {
            processor.prepareToPlay(44100.0, 512);
        }

        ~GraphHandlerFixture()
        {
            processor.releaseResources();
        }
    };

    stellarr::bridge::GraphHandler makeHandler(GraphHandlerFixture& fx,
                                               stellarr::test::MockBridgeEmitter& emitter)
    {
        stellarr::bridge::GraphHandlerContext ctx {
            fx.processor,
            fx.blockNodeMap,
            fx.blockPositions,
            fx.clipboardJson,
            emitter,
            [](const juce::String&) {},
            [] {},
            [] {}
        };
        return stellarr::bridge::GraphHandler(ctx);
    }
}

static bool testHandleAddBlockEmitsBlockAdded()
{
    printf("Test: handleAddBlock(plugin) emits blockAdded and updates map... ");

    GraphHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    auto json = juce::JSON::parse(R"({"type":"plugin","col":3,"row":2})");
    handler.handleAddBlock(json);

    if (!emitter.wasEmitted("blockAdded"))
    {
        fprintf(stderr, "  expected blockAdded event\n");
        printf("FAIL\n");
        return false;
    }
    if (fx.blockNodeMap.size() != 1)
    {
        fprintf(stderr, "  expected 1 block in map, got %zu\n", fx.blockNodeMap.size());
        printf("FAIL\n");
        return false;
    }
    if (fx.blockPositions.size() != 1)
    {
        fprintf(stderr, "  expected 1 entry in positions, got %zu\n", fx.blockPositions.size());
        printf("FAIL\n");
        return false;
    }

    auto& pos = fx.blockPositions.begin()->second;
    if (pos.first != 3 || pos.second != 2)
    {
        fprintf(stderr, "  expected position (3,2), got (%d,%d)\n", pos.first, pos.second);
        printf("FAIL\n");
        return false;
    }

    auto* rec = emitter.firstOf("blockAdded");
    if (rec == nullptr || rec->getDetail() == nullptr)
    {
        printf("FAIL (missing recorded event)\n");
        return false;
    }
    if (rec->getDetail()->getProperty("type").toString() != "plugin")
    {
        printf("FAIL (wrong type in payload)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testHandleAddBlockUnknownTypeIsNoop()
{
    printf("Test: handleAddBlock with unknown type does not emit or crash... ");

    GraphHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    auto json = juce::JSON::parse(R"({"type":"bogus","col":0,"row":0})");
    handler.handleAddBlock(json);

    if (emitter.wasEmitted("blockAdded"))
    {
        fprintf(stderr, "  blockAdded should not have fired\n");
        printf("FAIL\n");
        return false;
    }
    if (!fx.blockNodeMap.empty())
    {
        printf("FAIL (block map should stay empty)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testHandleRemoveBlockEmitsBlockRemoved()
{
    printf("Test: handleRemoveBlock removes from map and emits blockRemoved... ");

    GraphHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    // Add a block first, then clear emitter so we can isolate the
    // remove-time emit.
    handler.handleAddBlock(juce::JSON::parse(R"({"type":"plugin","col":1,"row":1})"));
    if (fx.blockNodeMap.size() != 1)
    {
        printf("FAIL (setup: expected 1 block in map)\n");
        return false;
    }
    auto blockId = fx.blockNodeMap.begin()->first;
    emitter.clear();

    auto* obj = new juce::DynamicObject();
    obj->setProperty("blockId", blockId);
    handler.handleRemoveBlock(juce::var(obj));

    if (!emitter.wasEmitted("blockRemoved"))
    {
        fprintf(stderr, "  expected blockRemoved event\n");
        printf("FAIL\n");
        return false;
    }
    if (!fx.blockNodeMap.empty())
    {
        fprintf(stderr, "  block map should be empty, has %zu\n", fx.blockNodeMap.size());
        printf("FAIL\n");
        return false;
    }
    if (!fx.blockPositions.empty())
    {
        fprintf(stderr, "  positions should be empty, has %zu\n", fx.blockPositions.size());
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testHandleMoveBlockUpdatesPositionsAndEmits()
{
    printf("Test: handleMoveBlock updates positions and emits blockMoved... ");

    GraphHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    handler.handleAddBlock(juce::JSON::parse(R"({"type":"plugin","col":0,"row":0})"));
    auto blockId = fx.blockNodeMap.begin()->first;
    emitter.clear();

    auto* obj = new juce::DynamicObject();
    obj->setProperty("blockId", blockId);
    obj->setProperty("col", 7);
    obj->setProperty("row", 4);
    handler.handleMoveBlock(juce::var(obj));

    if (!emitter.wasEmitted("blockMoved"))
    {
        fprintf(stderr, "  expected blockMoved event\n");
        printf("FAIL\n");
        return false;
    }
    auto& pos = fx.blockPositions[blockId];
    if (pos.first != 7 || pos.second != 4)
    {
        fprintf(stderr, "  expected (7,4) got (%d,%d)\n", pos.first, pos.second);
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    int failures = 0;

    if (!testHandleAddBlockEmitsBlockAdded())             ++failures;
    if (!testHandleAddBlockUnknownTypeIsNoop())           ++failures;
    if (!testHandleRemoveBlockEmitsBlockRemoved())        ++failures;
    if (!testHandleMoveBlockUpdatesPositionsAndEmits())   ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
