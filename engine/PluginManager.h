#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

struct ScanDirectory
{
    juce::String path;
    bool isDefault;
};

class PluginManager
{
public:
    PluginManager();

    void scanPlugins();

    const juce::KnownPluginList& getKnownPlugins() const { return knownPlugins; }
    const std::vector<ScanDirectory>& getScanDirectories() const { return scanDirectories; }

    void addScanDirectory(const juce::String& path);
    void removeScanDirectory(const juce::String& path);

    std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(
        const juce::String& pluginIdentifier, double sampleRate, int blockSize,
        juce::String& errorMessage);

private:
    void addDefaultDirectories();

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    std::vector<ScanDirectory> scanDirectories;
};
