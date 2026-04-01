#include "PluginManager.h"

PluginManager::PluginManager()
{
#if JUCE_PLUGINHOST_VST3
    formatManager.addFormat(new juce::VST3PluginFormat());
#endif
#if JUCE_PLUGINHOST_AU
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
#endif
    addDefaultDirectories();
}

void PluginManager::addDefaultDirectories()
{
    auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    // macOS standard plugin directories
    scanDirectories.push_back({home.getChildFile("Library/Audio/Plug-Ins/VST3").getFullPathName(), true});
    scanDirectories.push_back({"/Library/Audio/Plug-Ins/VST3", true});
    scanDirectories.push_back({home.getChildFile("Library/Audio/Plug-Ins/Components").getFullPathName(), true});
    scanDirectories.push_back({"/Library/Audio/Plug-Ins/Components", true});
}

void PluginManager::scanPlugins()
{
    for (auto& dir : scanDirectories)
    {
        auto directory = juce::File(dir.path);
        if (!directory.isDirectory())
            continue;

        for (auto* format : formatManager.getFormats())
        {
            juce::PluginDirectoryScanner scanner(
                knownPlugins, *format, juce::FileSearchPath(dir.path),
                true, juce::File{});

            juce::String pluginName;
            while (scanner.scanNextFile(true, pluginName))
            {
            }
        }
    }
}

void PluginManager::addScanDirectory(const juce::String& path)
{
    for (auto& dir : scanDirectories)
        if (dir.path == path) return;

    scanDirectories.push_back({path, false});
}

void PluginManager::removeScanDirectory(const juce::String& path)
{
    scanDirectories.erase(
        std::remove_if(scanDirectories.begin(), scanDirectories.end(),
            [&](const ScanDirectory& d) { return !d.isDefault && d.path == path; }),
        scanDirectories.end());
}

std::unique_ptr<juce::AudioPluginInstance> PluginManager::createPluginInstance(
    const juce::String& pluginIdentifier, double sampleRate, int blockSize,
    juce::String& errorMessage)
{
    for (auto& desc : knownPlugins.getTypes())
    {
        if (desc.createIdentifierString() == pluginIdentifier)
            return formatManager.createPluginInstance(desc, sampleRate, blockSize, errorMessage);
    }

    errorMessage = "Plugin not found: " + pluginIdentifier;
    return nullptr;
}
