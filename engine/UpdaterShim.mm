#include "UpdaterShim.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Sparkle/Sparkle.h>

// The user-driver object Sparkle talks to. All protocol methods run on the
// main thread. We funnel every transition into a shared C++ state struct and
// invoke the C++ callback so the bridge can serialise onto the UI wire.
@interface StellarrSparkleDriver : NSObject <SPUUserDriver>
@property (nonatomic, copy) void (^onState)(stellarr::update::State);

// Pending replies. Sparkle's protocol is dialog-shaped: when an update is
// found or ready to install, it hands us a reply block and expects us to
// invoke it when the user decides. Our custom UI captures the user's answer
// via installUpdate()/dismiss, so we stash the latest pending reply here.
@property (nonatomic, copy) void (^pendingUpdateFoundReply)(SPUUserUpdateChoice);
@property (nonatomic, copy) void (^pendingReadyToInstallReply)(SPUUserUpdateChoice);

// Snapshot of the last-known state so partial transitions (progress ticks)
// don't wipe the version/releaseNotesUrl we captured earlier.
@property (nonatomic, assign) stellarr::update::State currentState;

// Expected bytes so ongoing downloads can compute a 0..1 fraction.
@property (nonatomic, assign) uint64_t expectedContentLength;
@property (nonatomic, assign) uint64_t receivedContentLength;
@end

@implementation StellarrSparkleDriver

- (void)emit
{
    if (self.onState) self.onState(self.currentState);
}

- (void)reset
{
    stellarr::update::State empty;
    self.currentState = empty;
    self.pendingUpdateFoundReply = nil;
    self.pendingReadyToInstallReply = nil;
    self.expectedContentLength = 0;
    self.receivedContentLength = 0;
}

// ── Permission request ────────────────────────────────────────────────
// We already opted in via Info.plist (SUEnableAutomaticChecks), so decline
// the runtime prompt. Sparkle still honours our plist choice.
- (void)showUpdatePermissionRequest:(SPUUpdatePermissionRequest *)request
                              reply:(void (^)(SUUpdatePermissionResponse *))reply
{
    (void) request;
    reply([[SUUpdatePermissionResponse alloc]
           initWithAutomaticUpdateChecks:YES
                     sendSystemProfile:NO]);
}

// ── User-initiated check in progress ──────────────────────────────────
- (void)showUserInitiatedUpdateCheckWithCancellation:(void (^)(void))cancellation
{
    (void) cancellation;
    auto s = self.currentState;
    s.status = stellarr::update::Status::Checking;
    s.error.clear();
    self.currentState = s;
    [self emit];
}

// ── Update found — populate banner state, stash the reply ─────────────
- (void)showUpdateFoundWithAppcastItem:(SUAppcastItem *)appcastItem
                                 state:(SPUUserUpdateState *)state
                                 reply:(void (^)(SPUUserUpdateChoice))reply
{
    (void) state;
    stellarr::update::State s = self.currentState;
    s.status = stellarr::update::Status::Available;

    if (appcastItem.displayVersionString.length > 0)
        s.latestVersion = appcastItem.displayVersionString.UTF8String;
    else if (appcastItem.versionString.length > 0)
        s.latestVersion = appcastItem.versionString.UTF8String;

    if (appcastItem.date != nil) {
        NSISO8601DateFormatter* fmt = [[NSISO8601DateFormatter alloc] init];
        fmt.formatOptions = NSISO8601DateFormatWithInternetDateTime;
        s.releasedAt = [fmt stringFromDate:appcastItem.date].UTF8String;
    }

    if (appcastItem.contentLength > 0)
        s.sizeBytes = (long long) appcastItem.contentLength;

    if (appcastItem.releaseNotesURL != nil)
        s.releaseNotesUrl = appcastItem.releaseNotesURL.absoluteString.UTF8String;
    else if (appcastItem.infoURL != nil)
        s.releaseNotesUrl = appcastItem.infoURL.absoluteString.UTF8String;

    s.error.clear();
    self.currentState = s;
    self.pendingUpdateFoundReply = reply;
    [self emit];
}

// Release-notes assets — we surface the URL directly via releaseNotesUrl, so
// the download variants are no-ops.
- (void)showUpdateReleaseNotesWithDownloadData:(SPUDownloadData *)downloadData
{
    (void) downloadData;
}

- (void)showUpdateReleaseNotesFailedToDownloadWithError:(NSError *)error
{
    (void) error;
}

// ── No update ─────────────────────────────────────────────────────────
- (void)showUpdateNotFoundWithError:(NSError *)error
                   acknowledgement:(void (^)(void))acknowledgement
{
    (void) error;
    auto s = self.currentState;
    s.status = stellarr::update::Status::NoUpdate;
    s.error.clear();
    self.currentState = s;
    acknowledgement();
    [self emit];
}

// ── Error ─────────────────────────────────────────────────────────────
- (void)showUpdaterError:(NSError *)error acknowledgement:(void (^)(void))acknowledgement
{
    auto s = self.currentState;
    s.status = stellarr::update::Status::Error;
    const char* msg = error.localizedDescription.UTF8String;
    s.error = msg != nullptr ? std::string(msg) : std::string("Update failed");
    self.currentState = s;
    acknowledgement();
    [self emit];
}

// ── Download lifecycle ────────────────────────────────────────────────
- (void)showDownloadInitiatedWithCancellation:(void (^)(void))cancellation
{
    (void) cancellation;
    auto s = self.currentState;
    s.status = stellarr::update::Status::Downloading;
    s.downloadProgress = 0.0;
    s.error.clear();
    self.currentState = s;
    self.pendingUpdateFoundReply = nil;   // consumed
    self.expectedContentLength = 0;
    self.receivedContentLength = 0;
    [self emit];
}

- (void)showDownloadDidReceiveExpectedContentLength:(uint64_t)expectedContentLength
{
    self.expectedContentLength = expectedContentLength;
    if (self.currentState.sizeBytes <= 0 && expectedContentLength > 0) {
        auto s = self.currentState;
        s.sizeBytes = (long long) expectedContentLength;
        self.currentState = s;
        [self emit];
    }
}

- (void)showDownloadDidReceiveDataOfLength:(uint64_t)length
{
    self.receivedContentLength += length;
    if (self.expectedContentLength == 0) return;

    const double fraction = (double) self.receivedContentLength
                          / (double) self.expectedContentLength;
    auto s = self.currentState;
    s.status = stellarr::update::Status::Downloading;
    s.downloadProgress = fraction > 1.0 ? 1.0 : fraction;
    self.currentState = s;
    [self emit];
}

- (void)showDownloadDidStartExtractingUpdate
{
    // Remain in Downloading; Sparkle is now unpacking the DMG/ZIP.
    auto s = self.currentState;
    s.status = stellarr::update::Status::Downloading;
    self.currentState = s;
    [self emit];
}

- (void)showExtractionReceivedProgress:(double)progress
{
    auto s = self.currentState;
    s.status = stellarr::update::Status::Downloading;
    s.downloadProgress = progress < 0.0 ? 0.0 : (progress > 1.0 ? 1.0 : progress);
    self.currentState = s;
    [self emit];
}

// ── Ready to install ──────────────────────────────────────────────────
- (void)showReadyToInstallAndRelaunch:(void (^)(SPUUserUpdateChoice))reply
{
    auto s = self.currentState;
    s.status = stellarr::update::Status::Ready;
    s.downloadProgress = 1.0;
    self.currentState = s;
    self.pendingReadyToInstallReply = reply;
    [self emit];
}

// ── Installing (kernel in motion; typically followed by termination) ──
- (void)showInstallingUpdateWithApplicationTerminated:(BOOL)applicationTerminated
                         retryTerminatingApplication:(void (^)(void))retryTerminatingApplication
{
    (void) applicationTerminated;
    (void) retryTerminatingApplication;
    // No UI to show; Sparkle will relaunch us shortly.
}

- (void)showUpdateInstalledAndRelaunched:(BOOL)relaunched
                         acknowledgement:(void (^)(void))acknowledgement
{
    (void) relaunched;
    acknowledgement();
}

// ── Tear-down ─────────────────────────────────────────────────────────
- (void)dismissUpdateInstallation
{
    [self reset];
    [self emit];
}

- (void)showUpdateInFocus
{
    [NSApp activateIgnoringOtherApps:YES];
}

@end


namespace stellarr::update {

struct Shim::Impl {
    SPUUpdater*              updater { nil };
    StellarrSparkleDriver*   driver  { nil };
    StateCallback            callback;

    void emit(const State& s) {
        if (callback) callback(s);
    }
};

Shim::Shim() : impl(new Impl)
{
    impl->driver = [[StellarrSparkleDriver alloc] init];

    Impl* rawImpl = impl;
    impl->driver.onState = ^(State s) {
        rawImpl->emit(s);
    };

    NSBundle* mainBundle = [NSBundle mainBundle];
    impl->updater = [[SPUUpdater alloc]
                     initWithHostBundle:mainBundle
                      applicationBundle:mainBundle
                             userDriver:impl->driver
                               delegate:nil];

    NSError* startErr = nil;
    if (![impl->updater startUpdater:&startErr]) {
        NSLog(@"[Stellarr] Sparkle updater failed to start: %@", startErr);
        State s;
        s.status = Status::Error;
        const char* msg = startErr.localizedDescription.UTF8String;
        s.error = msg != nullptr ? std::string(msg) : std::string("Sparkle start failed");
        impl->driver.currentState = s;
    }
}

Shim::~Shim()
{
    if (impl != nullptr) {
        impl->updater = nil;
        impl->driver  = nil;
    }
    delete impl;
    impl = nullptr;
}

void Shim::setOnStateChanged(StateCallback cb)
{
    impl->callback = std::move(cb);
    impl->emit(impl->driver.currentState);
}

void Shim::checkForUpdates()
{
    [impl->updater checkForUpdates];
}

void Shim::installUpdate()
{
    // Most common: user just saw the "Update available" banner and clicked
    // Install. Consume the stashed update-found reply.
    if (impl->driver.pendingUpdateFoundReply != nil) {
        auto reply = impl->driver.pendingUpdateFoundReply;
        impl->driver.pendingUpdateFoundReply = nil;
        reply(SPUUserUpdateChoiceInstall);
        return;
    }

    // Download has completed; the banner now says "Restart & install".
    if (impl->driver.pendingReadyToInstallReply != nil) {
        auto reply = impl->driver.pendingReadyToInstallReply;
        impl->driver.pendingReadyToInstallReply = nil;
        reply(SPUUserUpdateChoiceInstall);
        return;
    }

    // No pending prompt: kick a fresh check so the flow can start from the
    // top if the user clicks Install speculatively.
    [impl->updater checkForUpdates];
}

void Shim::openReleaseNotes()
{
    const auto& url = impl->driver.currentState.releaseNotesUrl;
    if (url.empty()) return;
    NSString* s = [NSString stringWithUTF8String:url.c_str()];
    NSURL* u = [NSURL URLWithString:s];
    if (u != nil) [[NSWorkspace sharedWorkspace] openURL:u];
}

} // namespace stellarr::update
