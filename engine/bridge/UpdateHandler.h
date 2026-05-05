#pragma once
#include <memory>
#include "IBridgeEmitter.h"

namespace juce { class var; }
namespace stellarr::update { class Shim; struct State; }

namespace stellarr::bridge
{
    // Per-handler context wired up by StellarrBridge at construction. Keeping
    // the dependencies in a struct (rather than a long ctor parameter list)
    // mirrors the pattern that the remaining Phase 7 commits will adopt for
    // the other bridge handlers.
    struct UpdateHandlerContext
    {
        IBridgeEmitter& emit;
    };

    // Sparkle update bridge handler. Translates the UI's update/* events into
    // calls on the underlying stellarr::update::Shim and re-emits the shim's
    // state changes back to the UI.
    class UpdateHandler
    {
    public:
        explicit UpdateHandler(UpdateHandlerContext ctx);
        ~UpdateHandler();

        void handleCheck();
        void handleInstall();
        void handleOpenReleaseNotes(const juce::var& json);

    private:
        void ensureShim();
        void sendState(const stellarr::update::State& state);

        UpdateHandlerContext ctx;
        std::unique_ptr<stellarr::update::Shim> shim;
    };
}
