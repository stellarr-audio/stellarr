#include "TestUtils.h"
#include "blocks/GainBlock.h"
#include "blocks/InputBlock.h"
#include "blocks/VstBlock.h"

static bool testVstBlockPassThrough()
{
    printf("Test: VstBlock with no plugin passes audio... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto vst = std::make_unique<stellarr::VstBlock>();

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(vst));
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

int main()
{
    int failures = 0;

    if (!testVstBlockPassThrough())     ++failures;
    if (!testInputBlockTestTone())      ++failures;
    if (!testInputBlockTestToneReset()) ++failures;
    if (!testSerialisation())           ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
