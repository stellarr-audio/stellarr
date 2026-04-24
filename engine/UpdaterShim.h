#pragma once

#include <functional>
#include <string>

// C++ facade over Sparkle's SPUUpdater + a custom SPUUserDriver. Owns the
// update lifecycle and exposes it as a small State struct that the bridge
// layer can serialise onto the UI wire.
//
// All state transitions are emitted via the callback registered through
// setOnStateChanged. Callbacks fire on the main thread (SPUUserDriver is
// main-thread only by contract).
namespace stellarr::update {

enum class Status {
    Idle,
    Checking,
    Available,
    NoUpdate,
    Downloading,
    Ready,
    Error,
};

struct State {
    Status      status { Status::Idle };
    std::string latestVersion;
    std::string releasedAt;        // ISO-8601 date string, best-effort
    long long   sizeBytes { 0 };
    std::string releaseNotesUrl;
    double      downloadProgress { 0.0 };  // 0..1 when downloading
    std::string error;
};

class Shim {
public:
    using StateCallback = std::function<void(const State&)>;

    Shim();
    ~Shim();

    Shim(const Shim&) = delete;
    Shim& operator=(const Shim&) = delete;

    // Register a callback that is invoked on every state change. On
    // registration the current state is emitted synchronously so the UI can
    // paint an initial view.
    void setOnStateChanged(StateCallback cb);

    // Triggered by the user pressing the "Check for updates" button.
    void checkForUpdates();

    // Triggered by the user pressing the "Install update" or
    // "Restart & install" button. Resolves whichever Sparkle prompt is
    // currently waiting on a reply (either the update-found prompt or the
    // ready-to-install prompt).
    void installUpdate();

    // Open a release-notes URL in the user's default browser. If url is
    // empty, falls back to the URL stashed by the last update-found event
    // (may itself be empty if no update has been announced).
    void openReleaseNotes(const std::string& url = "");

private:
    struct Impl;
    Impl* impl { nullptr };
};

} // namespace stellarr::update
