#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "StellarrEditor.h"
#include "StellarrProcessor.h"
#include "Telemetry.h"

namespace juce
{

class StellarrFilterWindow final : public StandaloneFilterWindow,
                                   private ChangeListener
{
public:
    StellarrFilterWindow(const String& title,
                         Colour backgroundColour,
                         std::unique_ptr<StandalonePluginHolder> holder)
        : StandaloneFilterWindow(title, backgroundColour, std::move(holder))
    {
        // Save audio device settings immediately whenever they change,
        // so they survive a crash during the session.
        pluginHolder->deviceManager.addChangeListener(this);
    }

    ~StellarrFilterWindow() override
    {
        pluginHolder->deviceManager.removeChangeListener(this);
    }

private:
    void changeListenerCallback(ChangeBroadcaster*) override
    {
        pluginHolder->saveAudioDeviceState();
    }

    void buttonClicked(Button* button) override
    {
        PopupMenu m;
        m.addItem(1, TRANS("Audio/MIDI Settings..."));
        m.addSeparator();
        m.addItem(2, TRANS("Save current state..."));
        m.addItem(3, TRANS("Load a saved state..."));
        m.addSeparator();
        m.addItem(4, TRANS("Reset to default state"));

#if JUCE_DEBUG
        m.addSeparator();

        bool devToolsOn = false;
        if (auto* editor = dynamic_cast<StellarrEditor*>(
                getAudioProcessor()->getActiveEditor()))
            devToolsOn = editor->isDevToolsEnabled();

        m.addItem(5, TRANS("Enable DevTools"), true, devToolsOn);
#endif

        m.showMenuAsync(PopupMenu::Options().withTargetComponent(button),
                        ModalCallbackFunction::forComponent(menuCallback, this));
    }

    static void menuCallback(int result, StellarrFilterWindow* window)
    {
        if (window == nullptr || result == 0)
            return;

        if (result == 5)
        {
            if (auto* editor = dynamic_cast<StellarrEditor*>(
                    window->getAudioProcessor()->getActiveEditor()))
                editor->toggleDevTools();
        }
        else
        {
            window->handleMenuResult(result);
        }
    }
};

class StellarrStandaloneApp final : public JUCEApplication
{
public:
    StellarrStandaloneApp()
    {
        PropertiesFile::Options options;
        options.applicationName     = CharPointer_UTF8(JucePlugin_Name);
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName          = "";
        appProperties.setStorageParameters(options);
    }

    const String getApplicationName() override           { return CharPointer_UTF8(JucePlugin_Name); }
    const String getApplicationVersion() override        { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override            { return true; }
    void anotherInstanceStarted(const String&) override  {}

    void initialise(const String&) override
    {
        stellarr::Telemetry::init(&appProperties);

        mainWindow.reset(createWindow());

        if (mainWindow != nullptr)
        {
            if (auto* proc = dynamic_cast<StellarrProcessor*>(mainWindow->getAudioProcessor()))
                proc->setAppProperties(&appProperties);

            mainWindow->setVisible(true);
        }
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
        stellarr::Telemetry::close();
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
            mainWindow->pluginHolder->savePluginState();

        if (ModalComponentManager::getInstance()->cancelAllModalComponents())
            Timer::callAfterDelay(100, []() { JUCEApplication::getInstance()->systemRequestedQuit(); });
        else
            quit();
    }

private:
    StellarrFilterWindow* createWindow()
    {
        if (Desktop::getInstance().getDisplays().displays.isEmpty())
            return nullptr;

        const Array<StandalonePluginHolder::PluginInOuts> channelConfig;

        // JUCE mutes audio input by default in standalone apps — disable that
        // on first launch only, so the user's later preference is preserved.
        if (!appProperties.getUserSettings()->containsKey("shouldMuteInput"))
            appProperties.getUserSettings()->setValue("shouldMuteInput", false);

        auto holder = std::make_unique<StandalonePluginHolder>(
            appProperties.getUserSettings(), false, String{},
            nullptr, channelConfig, false);

        // Set app properties on the processor before the editor is created,
        // so handleBridgeReady can access saved state.
        if (auto* proc = dynamic_cast<StellarrProcessor*>(holder->processor.get()))
            proc->setAppProperties(&appProperties);

        return new StellarrFilterWindow(
            getApplicationName(),
            LookAndFeel::getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId),
            std::move(holder));
    }

    ApplicationProperties appProperties;
    std::unique_ptr<StellarrFilterWindow> mainWindow;
};

} // namespace juce

START_JUCE_APPLICATION(juce::StellarrStandaloneApp)
