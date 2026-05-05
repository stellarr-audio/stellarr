#include <cstdio>
#include <juce_core/juce_core.h>
#include "../StellarrProcessor.h"
#include "../blocks/Block.h"
#include "../blocks/PluginBlock.h"
#include "../bridge/BridgeTypes.h"
#include "../bridge/EventNames.h"
#include "../bridge/ParamHandler.h"
#include "support/MockBridgeEmitter.h"

namespace events = stellarr::bridge::events;

// Per-handler isolation tests for ParamHandler. These construct a
// StellarrProcessor, add a real PluginBlock via its API, build a
// BlockNodeMap, and feed both into a ParamHandler with a
// MockBridgeEmitter — no StellarrBridge involved.

namespace
{
    // Tiny RAII-style helper that holds the processor, a single
    // PluginBlock-backed block, and the BlockNodeMap connecting them.
    struct ParamHandlerFixture
    {
        StellarrProcessor processor;
        stellarr::bridge::BlockNodeMap blockNodeMap;
        juce::String blockId;

        ParamHandlerFixture()
        {
            processor.prepareToPlay(44100.0, 512);

            auto block = std::make_unique<stellarr::PluginBlock>();
            blockId = block->getBlockId().toString();
            auto nodeId = processor.addBlock(std::move(block));
            blockNodeMap[blockId] = nodeId;
        }

        ~ParamHandlerFixture()
        {
            processor.releaseResources();
        }
    };

    // Construct a ParamHandler over the fixture with no-op
    // cross-handler callbacks (this commit's tests don't exercise the
    // delete / active-state-change paths).
    stellarr::bridge::ParamHandler makeHandler(ParamHandlerFixture& fx,
                                               stellarr::test::MockBridgeEmitter& emitter)
    {
        stellarr::bridge::ParamHandlerContext ctx {
            fx.processor,
            fx.blockNodeMap,
            emitter,
            [](const juce::String&, int) {},
            [](const juce::String&, int) {}
        };
        return stellarr::bridge::ParamHandler(ctx);
    }
}

static bool testHandleSetBlockMixUpdatesAndEmits()
{
    printf("Test: handleSetBlockMix sets value and emits blockMixChanged... ");

    ParamHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    auto* obj = new juce::DynamicObject();
    obj->setProperty("blockId", fx.blockId);
    obj->setProperty("mix", 0.5);
    handler.handleSetBlockMix(juce::var(obj));

    if (!emitter.wasEmitted(events::BlockMixChanged))
    {
        fprintf(stderr, "  expected blockMixChanged emit\n");
        printf("FAIL\n");
        return false;
    }

    // Verify the new mix value made it onto the block itself.
    auto* node = fx.processor.getGraph().getNodeForId(fx.blockNodeMap[fx.blockId]);
    auto* block = node ? dynamic_cast<stellarr::Block*>(node->getProcessor()) : nullptr;
    if (block == nullptr || std::abs(block->getMix() - 0.5f) > 0.001f)
    {
        fprintf(stderr, "  expected block->getMix() ~0.5, got %f\n",
                block ? static_cast<double>(block->getMix()) : -1.0);
        printf("FAIL\n");
        return false;
    }

    // The emitted event payload should carry the new mix value.
    auto* rec = emitter.firstOf(events::BlockMixChanged);
    if (rec == nullptr || rec->getDetail() == nullptr)
    {
        printf("FAIL (missing recorded event)\n");
        return false;
    }
    auto recordedMix = static_cast<double>(rec->getDetail()->getProperty("mix"));
    if (std::abs(recordedMix - 0.5) > 0.001)
    {
        fprintf(stderr, "  expected emitted mix ~0.5, got %f\n", recordedMix);
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testHandleSetBlockBypassModeUpdatesAndEmits()
{
    printf("Test: handleSetBlockBypassMode applies new mode and emits... ");

    ParamHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    auto* obj = new juce::DynamicObject();
    obj->setProperty("blockId", fx.blockId);
    obj->setProperty("bypassMode", "muteIn");
    handler.handleSetBlockBypassMode(juce::var(obj));

    if (!emitter.wasEmitted(events::BlockBypassModeChanged))
    {
        fprintf(stderr, "  expected blockBypassModeChanged emit\n");
        printf("FAIL\n");
        return false;
    }

    auto* node = fx.processor.getGraph().getNodeForId(fx.blockNodeMap[fx.blockId]);
    auto* block = node ? dynamic_cast<stellarr::Block*>(node->getProcessor()) : nullptr;
    if (block == nullptr || block->getBypassMode() != stellarr::BypassMode::muteIn)
    {
        fprintf(stderr, "  expected BypassMode::muteIn\n");
        printf("FAIL\n");
        return false;
    }

    auto* rec = emitter.firstOf(events::BlockBypassModeChanged);
    if (rec == nullptr || rec->getDetail() == nullptr)
    {
        printf("FAIL (missing recorded event)\n");
        return false;
    }
    if (rec->getDetail()->getProperty("bypassMode").toString() != "muteIn")
    {
        printf("FAIL (wrong mode in payload)\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testHandleSetBlockMixUnknownBlockIsNoop()
{
    printf("Test: handleSetBlockMix on unknown blockId does not emit or crash... ");

    ParamHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    auto* obj = new juce::DynamicObject();
    obj->setProperty("blockId", "nonexistent-block");
    obj->setProperty("mix", 0.42);
    handler.handleSetBlockMix(juce::var(obj));

    if (emitter.wasEmitted(events::BlockMixChanged))
    {
        fprintf(stderr, "  no blockMixChanged should have been emitted\n");
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

    if (!testHandleSetBlockMixUpdatesAndEmits())          ++failures;
    if (!testHandleSetBlockBypassModeUpdatesAndEmits())   ++failures;
    if (!testHandleSetBlockMixUnknownBlockIsNoop())       ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
