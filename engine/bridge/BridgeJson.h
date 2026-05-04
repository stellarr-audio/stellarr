#pragma once
#include <juce_core/juce_core.h>

namespace stellarr::bridge::json
{
    // Returns the DynamicObject* if json is an object payload, else nullptr.
    inline juce::DynamicObject* getObj(const juce::var& json)
    {
        return json.getDynamicObject();
    }

    // Read an int property; returns fallback if absent (void).
    inline int getOptInt(const juce::DynamicObject& obj,
                         const juce::Identifier& key,
                         int fallback)
    {
        auto v = obj.getProperty(key);
        return v.isVoid() ? fallback : static_cast<int>(v);
    }

    // Read a float property; returns fallback if absent. Caller passes the
    // sentinel explicitly (e.g. std::numeric_limits<float>::quiet_NaN()) so
    // the contract is visible at the call site rather than hidden in a default.
    inline float getOptFloat(const juce::DynamicObject& obj,
                             const juce::Identifier& key,
                             float fallback)
    {
        auto v = obj.getProperty(key);
        return v.isVoid() ? fallback : static_cast<float>(static_cast<double>(v));
    }

    // Read an int property and clamp to [lo, hi]; returns fallback if absent.
    inline int getOptIntClamped(const juce::DynamicObject& obj,
                                const juce::Identifier& key,
                                int lo, int hi, int fallback)
    {
        auto v = obj.getProperty(key);
        return v.isVoid() ? fallback : juce::jlimit(lo, hi, static_cast<int>(v));
    }

    // Read a string property; returns fallback if absent (void).
    inline juce::String getOptString(const juce::DynamicObject& obj,
                                     const juce::Identifier& key,
                                     const juce::String& fallback)
    {
        auto v = obj.getProperty(key);
        return v.isVoid() ? fallback : v.toString();
    }
}
