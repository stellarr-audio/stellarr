#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class PluginWindow final : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioProcessorEditor* editor, const juce::String& title)
        : DocumentWindow(title, juce::Colours::black, DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(false, false);
        setContentOwned(editor, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
