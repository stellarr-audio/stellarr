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

    void initialise(const String& commandLine) override
    {
        stellarr::Telemetry::init(&appProperties);

        // Parse --screenshot <config> <index> args
        auto args = StringArray::fromTokens(commandLine, true);
        int screenshotArgIdx = args.indexOf("--screenshot");
        if (screenshotArgIdx >= 0 && screenshotArgIdx + 2 < args.size())
        {
            screenshotConfigPath = args[screenshotArgIdx + 1];
            screenshotIndex = args[screenshotArgIdx + 2].getIntValue();
        }

        mainWindow.reset(createWindow());

        if (mainWindow != nullptr)
        {
            if (auto* proc = dynamic_cast<StellarrProcessor*>(mainWindow->getAudioProcessor()))
                proc->setAppProperties(&appProperties);

            mainWindow->setVisible(true);

            // Apply window size and pass screenshot config to bridge
            if (screenshotConfigPath.isNotEmpty())
            {
                auto configFile = File(screenshotConfigPath);
                if (configFile.existsAsFile())
                {
                    auto json = JSON::parse(configFile.loadFileAsString());
                    if (auto* arr = json.getArray())
                    {
                        if (screenshotIndex >= 0 && screenshotIndex < arr->size())
                        {
                            auto entry = (*arr)[screenshotIndex];
                            if (auto* obj = entry.getDynamicObject())
                            {
                                int w = obj->getProperty("windowSize").getArray() ?
                                    static_cast<int>(obj->getProperty("windowSize").getArray()->getFirst()) : 1280;
                                int h = obj->getProperty("windowSize").getArray() && obj->getProperty("windowSize").getArray()->size() > 1 ?
                                    static_cast<int>((*obj->getProperty("windowSize").getArray())[1]) : 800;
                                mainWindow->setSize(w, h);
                                mainWindow->centreWithSize(w, h);

                                if (auto* editor = dynamic_cast<StellarrEditor*>(
                                        mainWindow->getAudioProcessor()->getActiveEditor()))
                                    editor->getBridge().setScreenshotConfig(entry);
                            }
                        }
                    }
                }
            }
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
    String screenshotConfigPath;
    int screenshotIndex = -1;
};

} // namespace juce

START_JUCE_APPLICATION(juce::StellarrStandaloneApp)
