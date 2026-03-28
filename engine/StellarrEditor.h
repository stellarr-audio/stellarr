#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "StellarrBridge.h"

class StellarrProcessor;

class StellarrEditor final : public juce::AudioProcessorEditor
{
public:
    explicit StellarrEditor(StellarrProcessor&);
    ~StellarrEditor() override;

    void resized() override;

private:
    static juce::String getMimeType(const juce::File& file);

    StellarrBridge bridge;
    std::unique_ptr<juce::WebBrowserComponent> webView;
};
