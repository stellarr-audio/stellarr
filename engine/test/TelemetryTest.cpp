#include "TestUtils.h"
#include "Telemetry.h"

static std::unique_ptr<juce::ApplicationProperties> createTempProps()
{
    auto props = std::make_unique<juce::ApplicationProperties>();
    juce::PropertiesFile::Options options;
    options.applicationName = "StellarrTelemetryTest";
    options.filenameSuffix = ".test";
    options.folderName = juce::File::getSpecialLocation(
        juce::File::tempDirectory).getFullPathName();
    props->setStorageParameters(options);
    return props;
}

static bool testDefaultDisabled()
{
    printf("Test: telemetry disabled by default... ");

    auto props = createTempProps();

    if (stellarr::Telemetry::isEnabled(props.get()))
    {
        fprintf(stderr, "  expected disabled by default\n");
        printf("FAIL\n");
        return false;
    }

    props->closeFiles();
    printf("PASS\n");
    return true;
}

static bool testEnableAndPersist()
{
    printf("Test: telemetry can be enabled and persists... ");

    auto props = createTempProps();

    stellarr::Telemetry::setEnabled(props.get(), true);

    if (!stellarr::Telemetry::isEnabled(props.get()))
    {
        fprintf(stderr, "  expected enabled after setEnabled(true)\n");
        printf("FAIL\n");
        return false;
    }

    props->closeFiles();
    printf("PASS\n");
    return true;
}

static bool testDisableAfterEnable()
{
    printf("Test: telemetry can be disabled after enabling... ");

    auto props = createTempProps();

    stellarr::Telemetry::setEnabled(props.get(), true);
    stellarr::Telemetry::setEnabled(props.get(), false);

    if (stellarr::Telemetry::isEnabled(props.get()))
    {
        fprintf(stderr, "  expected disabled after setEnabled(false)\n");
        printf("FAIL\n");
        return false;
    }

    props->closeFiles();
    printf("PASS\n");
    return true;
}

static bool testNullPropsSafe()
{
    printf("Test: null ApplicationProperties does not crash... ");

    if (stellarr::Telemetry::isEnabled(nullptr))
    {
        fprintf(stderr, "  expected false for null props\n");
        printf("FAIL\n");
        return false;
    }

    stellarr::Telemetry::setEnabled(nullptr, true); // should not crash

    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    if (!testDefaultDisabled())    ++failures;
    if (!testEnableAndPersist())   ++failures;
    if (!testDisableAfterEnable()) ++failures;
    if (!testNullPropsSafe())      ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
