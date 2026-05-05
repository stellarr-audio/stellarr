#include "UpdateHandler.h"
#include "../UpdaterShim.h"
#include <juce_core/juce_core.h>

// Software update bridge handlers. These are thin translators between the
// UI's update/* events and the Sparkle-backed shim.

namespace stellarr::bridge {

namespace {

juce::String statusToString(stellarr::update::Status s)
{
    using S = stellarr::update::Status;
    switch (s)
    {
        case S::Idle:        return "idle";
        case S::Checking:    return "checking";
        case S::Available:   return "available";
        case S::NoUpdate:    return "no-update";
        case S::Downloading: return "downloading";
        case S::Ready:       return "ready";
        case S::Error:       return "error";
    }
    return "idle";
}

} // namespace

UpdateHandler::UpdateHandler(UpdateHandlerContext c) : ctx(c) {}
UpdateHandler::~UpdateHandler() = default;

void UpdateHandler::ensureShim()
{
    if (shim != nullptr) return;

    shim = std::make_unique<stellarr::update::Shim>();
    shim->setOnStateChanged([this](const stellarr::update::State& s)
    {
        sendState(s);
    });
}

void UpdateHandler::sendState(const stellarr::update::State& state)
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("status",           statusToString(state.status));
    detail->setProperty("latestVersion",    juce::String(state.latestVersion));
    detail->setProperty("releasedAt",       juce::String(state.releasedAt));
    detail->setProperty("sizeBytes",        (juce::int64) state.sizeBytes);
    detail->setProperty("releaseNotesUrl",  juce::String(state.releaseNotesUrl));
    detail->setProperty("downloadProgress", state.downloadProgress);
    detail->setProperty("error",            juce::String(state.error));
    ctx.emit.emit("updateState", detail);
}

void UpdateHandler::handleCheck()
{
    ensureShim();
    shim->checkForUpdates();
}

void UpdateHandler::handleInstall()
{
    ensureShim();
    shim->installUpdate();
}

void UpdateHandler::handleOpenReleaseNotes(const juce::var& json)
{
    ensureShim();

    juce::String url;
    if (auto* obj = json.getDynamicObject())
        url = obj->getProperty("url").toString();

    shim->openReleaseNotes(url.toStdString());
}

} // namespace stellarr::bridge
