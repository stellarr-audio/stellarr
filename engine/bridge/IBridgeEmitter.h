#pragma once
#include <juce_core/juce_core.h>

namespace juce { class DynamicObject; }

namespace stellarr::bridge
{
    // Abstract interface for emitting events from the engine to the UI.
    // StellarrBridge implements this; tests use a recording mock to assert
    // emit calls without spinning up a real WebView.
    class IBridgeEmitter
    {
    public:
        virtual ~IBridgeEmitter() = default;

        // Emit an event asynchronously on the message thread.
        //
        // Ownership: the implementation takes ownership of `detail` by
        // wrapping it in a juce::var (ref-counted). Callers transfer
        // ownership and must NOT touch `detail` after the call. Pass
        // `new juce::DynamicObject()` directly; do not retain a separate
        // raw pointer.
        virtual void emit(const juce::String& eventName,
                          juce::DynamicObject* detail) = 0;

        // Emit synchronously. Caller must already be on the message thread.
        // Used for events that must reach JS BEFORE the message thread
        // proceeds with a long-running operation (e.g. presetLoadStarted
        // before plugin preload).
        //
        // Ownership semantics identical to emit(): implementation takes
        // ownership of `detail`.
        virtual void emitSync(const juce::String& eventName,
                              juce::DynamicObject* detail) = 0;
    };
}
