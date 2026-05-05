#include <cstdio>
#include <juce_core/juce_core.h>
#include "../StellarrProcessor.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"
#include "../bridge/BridgeTypes.h"
#include "../bridge/EventNames.h"
#include "../bridge/InputBlockHandler.h"
#include "support/MockBridgeEmitter.h"

namespace events = stellarr::bridge::events;

// Per-handler isolation tests for InputBlockHandler. The handler's
// tuner-related paths are inert (no filesystem, no network, no GUI), so
// they're a clean fit for unit-level coverage. Test-tone sample-pick paths
// touch the bundle samples directory and are deliberately out of scope.

namespace
{
    // Holds a processor with one InputBlock and one OutputBlock plus a
    // BlockNodeMap connecting them. setTunerEnabledOnAllBlocks iterates
    // the map and applies state to both block types, so we need both
    // present.
    struct InputBlockHandlerFixture
    {
        StellarrProcessor processor;
        stellarr::bridge::BlockNodeMap blockNodeMap;
        juce::String inputBlockId;
        juce::String outputBlockId;

        InputBlockHandlerFixture()
        {
            processor.prepareToPlay(44100.0, 512);

            auto inputBlock = std::make_unique<stellarr::InputBlock>();
            inputBlockId = inputBlock->getBlockId().toString();
            blockNodeMap[inputBlockId] = processor.addBlock(std::move(inputBlock));

            auto outputBlock = std::make_unique<stellarr::OutputBlock>();
            outputBlockId = outputBlock->getBlockId().toString();
            blockNodeMap[outputBlockId] = processor.addBlock(std::move(outputBlock));
        }

        ~InputBlockHandlerFixture()
        {
            processor.releaseResources();
        }

        stellarr::InputBlock* getInputBlock()
        {
            auto* node = processor.getGraph().getNodeForId(blockNodeMap[inputBlockId]);
            return node ? dynamic_cast<stellarr::InputBlock*>(node->getProcessor()) : nullptr;
        }

        stellarr::OutputBlock* getOutputBlock()
        {
            auto* node = processor.getGraph().getNodeForId(blockNodeMap[outputBlockId]);
            return node ? dynamic_cast<stellarr::OutputBlock*>(node->getProcessor()) : nullptr;
        }
    };

    stellarr::bridge::InputBlockHandler makeHandler(InputBlockHandlerFixture& fx,
                                                     stellarr::test::MockBridgeEmitter& emitter)
    {
        stellarr::bridge::InputBlockHandlerContext ctx { fx.processor, fx.blockNodeMap, emitter };
        return stellarr::bridge::InputBlockHandler(ctx);
    }
}

static bool testHandleSetTunerEnabledFlipsFlagAndApplies()
{
    printf("Test: handleSetTunerEnabled flips isTunerActive and applies to all blocks... ");

    InputBlockHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    if (handler.isTunerActive())
    {
        fprintf(stderr, "  expected tuner OFF on construction\n");
        printf("FAIL\n");
        return false;
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty("enabled", true);
    handler.handleSetTunerEnabled(juce::var(obj));

    if (!handler.isTunerActive())
    {
        fprintf(stderr, "  expected isTunerActive() == true after handleSetTunerEnabled(true)\n");
        printf("FAIL\n");
        return false;
    }

    auto* input = fx.getInputBlock();
    auto* output = fx.getOutputBlock();
    if (input == nullptr || output == nullptr || !input->isTunerEnabled() || !output->isTunerMuted())
    {
        fprintf(stderr, "  expected InputBlock::isTunerEnabled and OutputBlock::isTunerMuted\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testSetTunerEnabledOnAllBlocksTogglesBackOff()
{
    printf("Test: setTunerEnabledOnAllBlocks(false) clears state on all blocks... ");

    InputBlockHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    handler.setTunerEnabledOnAllBlocks(true);
    if (!handler.isTunerActive())
    {
        printf("FAIL (precondition)\n");
        return false;
    }

    handler.setTunerEnabledOnAllBlocks(false);
    if (handler.isTunerActive())
    {
        fprintf(stderr, "  expected isTunerActive() == false\n");
        printf("FAIL\n");
        return false;
    }

    auto* input = fx.getInputBlock();
    auto* output = fx.getOutputBlock();
    if (input == nullptr || output == nullptr || input->isTunerEnabled() || output->isTunerMuted())
    {
        fprintf(stderr, "  expected tuner state cleared on both blocks\n");
        printf("FAIL\n");
        return false;
    }

    printf("PASS\n");
    return true;
}

static bool testHandleToggleTestToneEmits()
{
    printf("Test: handleToggleTestTone toggles state and emits testToneChanged... ");

    InputBlockHandlerFixture fx;
    stellarr::test::MockBridgeEmitter emitter;
    auto handler = makeHandler(fx, emitter);

    auto* obj = new juce::DynamicObject();
    obj->setProperty("blockId", fx.inputBlockId);
    handler.handleToggleTestTone(juce::var(obj));

    if (!emitter.wasEmitted(events::InputTestToneChanged))
    {
        fprintf(stderr, "  expected testToneChanged emit\n");
        printf("FAIL\n");
        return false;
    }

    auto* input = fx.getInputBlock();
    if (input == nullptr || !input->isTestToneEnabled())
    {
        fprintf(stderr, "  expected isTestToneEnabled() == true after toggle\n");
        printf("FAIL\n");
        return false;
    }

    auto* rec = emitter.firstOf(events::InputTestToneChanged);
    if (rec == nullptr || rec->getDetail() == nullptr
        || !static_cast<bool>(rec->getDetail()->getProperty("enabled")))
    {
        fprintf(stderr, "  expected emit payload with enabled == true\n");
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

    if (!testHandleSetTunerEnabledFlipsFlagAndApplies()) ++failures;
    if (!testSetTunerEnabledOnAllBlocksTogglesBackOff()) ++failures;
    if (!testHandleToggleTestToneEmits())                ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
