#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include "../../bridge/IBridgeEmitter.h"

namespace stellarr::test
{
    // IBridgeEmitter implementation that records every emit call into a
    // vector of (eventName, detail) pairs. Use `wasEmitted(name)`,
    // `countOf(name)`, `firstOf(name)` etc. to verify what the handler
    // under test broadcast.
    //
    // The detail pointer is wrapped in juce::var on capture (matching the
    // production StellarrBridge::emit ownership semantics) so the recorded
    // data stays alive as long as the mock does, and inspection is safe.
    class MockBridgeEmitter : public stellarr::bridge::IBridgeEmitter
    {
    public:
        struct Recorded
        {
            juce::String eventName;
            juce::var data; // wraps the original DynamicObject*

            // Convenience: get the underlying DynamicObject* for property reads.
            juce::DynamicObject* getDetail() const { return data.getDynamicObject(); }
        };

        void emit(const juce::String& eventName, juce::DynamicObject* detail) override
        {
            calls.push_back({ eventName, juce::var(detail) });
        }

        void emitSync(const juce::String& eventName, juce::DynamicObject* detail) override
        {
            calls.push_back({ eventName, juce::var(detail) });
        }

        // ---- Test assertions ------------------------------------------

        // Number of calls (any event).
        size_t size() const { return calls.size(); }

        // Number of calls for a specific event name.
        size_t countOf(const juce::String& eventName) const
        {
            size_t n = 0;
            for (auto& c : calls)
                if (c.eventName == eventName) ++n;
            return n;
        }

        // Was an event emitted at least once?
        bool wasEmitted(const juce::String& eventName) const
        {
            return countOf(eventName) > 0;
        }

        // First recorded call (caller must ensure at least one exists).
        const Recorded& first() const { return calls.at(0); }

        // First recorded call for a specific event name. Returns nullptr
        // if none.
        const Recorded* firstOf(const juce::String& eventName) const
        {
            for (auto& c : calls)
                if (c.eventName == eventName) return &c;
            return nullptr;
        }

        // Drop all recorded calls (for setup vs assertion phase).
        void clear() { calls.clear(); }

        const std::vector<Recorded>& all() const { return calls; }

    private:
        std::vector<Recorded> calls;
    };
}
