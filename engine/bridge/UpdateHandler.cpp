#include "../StellarrBridge.h"
#include "../UpdaterShim.h"

// Software update bridge handlers. These are thin translators between the
// UI's update/* events and the Sparkle-backed shim.

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

void StellarrBridge::ensureUpdateShim()
{
    if (updateShim != nullptr) return;

    updateShim = std::make_unique<stellarr::update::Shim>();
    updateShim->setOnStateChanged([this](const stellarr::update::State& s)
    {
        sendUpdateState(s);
    });
}

void StellarrBridge::sendUpdateState(const stellarr::update::State& state)
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("status",           statusToString(state.status));
    detail->setProperty("latestVersion",    juce::String(state.latestVersion));
    detail->setProperty("releasedAt",       juce::String(state.releasedAt));
    detail->setProperty("sizeBytes",        (juce::int64) state.sizeBytes);
    detail->setProperty("releaseNotesUrl",  juce::String(state.releaseNotesUrl));
    detail->setProperty("downloadProgress", state.downloadProgress);
    detail->setProperty("error",            juce::String(state.error));
    emitToJs("updateState", detail);
}

void StellarrBridge::handleUpdateCheck()
{
    ensureUpdateShim();
    updateShim->checkForUpdates();
}

void StellarrBridge::handleUpdateInstall()
{
    ensureUpdateShim();
    updateShim->installUpdate();
}

void StellarrBridge::handleUpdateOpenReleaseNotes()
{
    ensureUpdateShim();
    updateShim->openReleaseNotes();
}
