#pragma once
#include <juce_core/juce_core.h>
#include <sentry.h>

namespace stellarr
{

class Telemetry
{
public:
    static bool isEnabled(juce::ApplicationProperties* props)
    {
        if (props == nullptr) return false;
        auto* settings = props->getUserSettings();
        return settings != nullptr && settings->getBoolValue("telemetryEnabled", false);
    }

    static void setEnabled(juce::ApplicationProperties* props, bool enabled)
    {
        if (props == nullptr) return;
        auto* settings = props->getUserSettings();
        if (settings != nullptr)
            settings->setValue("telemetryEnabled", enabled);

#ifdef STELLARR_SENTRY_DSN
        // Update consent in the running Sentry session
        if (enabled)
            sentry_user_consent_give();
        else
            sentry_user_consent_revoke();
#endif
    }

    static void init(juce::ApplicationProperties* props)
    {
#ifndef STELLARR_SENTRY_DSN
        juce::ignoreUnused(props);
        return;
#else
        auto options = sentry_options_new();
        sentry_options_set_dsn(options, STELLARR_SENTRY_DSN);

        auto release = juce::String("stellarr@") + JucePlugin_VersionString;
        sentry_options_set_release(options, release.toRawUTF8());

#ifdef NDEBUG
        sentry_options_set_environment(options, "production");
#else
        sentry_options_set_environment(options, "development");
#endif

        // Store crash database in app support directory
        auto dbPath = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("Stellarr")
            .getChildFile(".sentry-native")
            .getFullPathName();
        sentry_options_set_database_path(options, dbPath.toRawUTF8());

        // Require user consent — no data sent until user opts in
        sentry_options_set_require_user_consent(options, 1);

        sentry_init(options);

        // Apply stored preference
        if (isEnabled(props))
            sentry_user_consent_give();
        else
            sentry_user_consent_revoke();
#endif
    }

    static void close()
    {
#ifdef STELLARR_SENTRY_DSN
        sentry_close();
#endif
    }
};

} // namespace stellarr
