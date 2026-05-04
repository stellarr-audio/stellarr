#include "../StellarrProcessor.h"
#include "SceneCapture.h"

// -- Preset management --------------------------------------------------------

void StellarrBridge::setPresetFromFile(const juce::File& file)
{
    lastPresetFile = file;
    presetDirectory = file.getParentDirectory();
    handleGetPresetList();

    currentPresetIndex = -1;
    for (int i = 0; i < presetFiles.size(); ++i)
    {
        if (presetFiles[i] == file.getFileName())
        {
            currentPresetIndex = i;
            break;
        }
    }

    sendPresetList();
    persistPresetInfo();
}

void StellarrBridge::persistPresetInfo()
{
    if (appProperties == nullptr) return;

    auto* settings = appProperties->getUserSettings();
    settings->setValue("lastPresetDirectory", presetDirectory.getFullPathName());
    settings->setValue("lastPresetIndex", currentPresetIndex);
    settings->setValue("lastPresetFile", lastPresetFile.getFullPathName());

    // Persist global MIDI mappings (preset change, tuner toggle)
    if (processor != nullptr)
    {
        auto globalJson = juce::JSON::toString(processor->getMidiMapper().globalMappingsToJson());
        settings->setValue("globalMidiMappings", globalJson);
    }

    appProperties->saveIfNeeded();
}

void StellarrBridge::handleNewSession()
{
    clearGraph();

    auto inputJson = juce::JSON::parse(R"({"type":"input","col":0,"row":2})");
    auto outputJson = juce::JSON::parse(R"({"type":"output","col":11,"row":2})");
    handleAddBlock(inputJson);
    handleAddBlock(outputJson);

    lastPresetFile = juce::File{};
    currentPresetIndex = -1;
    scenes.clear();
    Scene defaultScene;
    defaultScene.name = "Scene 1";
    captureIntoScene(defaultScene, blockNodeMap, processor->getGraph());
    scenes.push_back(defaultScene);
    activeSceneIndex = 0;

    // Clear preset-level MIDI mappings
    if (processor != nullptr)
    {
        processor->getMidiMapper().loadPresetMappings(juce::var());
        emitMidiMappings();
    }

    persistPresetInfo();

    // Reset grid size to the UI default when starting a new session so the
    // new preset doesn't inherit the previous session's custom dimensions.
    gridCols = 12;
    gridRows = 5;

    sendGraphState();
    emitGridState();
    sendPresetList();
}

void StellarrBridge::handleSaveSession()
{
    if (processor == nullptr) return;

    juce::MessageManager::callAsync([this]()
    {
        juce::FileChooser chooser("Save Preset", presetDirectory, "*.stellarr");

        if (!chooser.browseForFileToSave(true)) return;

        auto file = chooser.getResult().withFileExtension("stellarr");
        auto session = serialiseSession();
        auto jsonStr = juce::JSON::toString(session);
        file.replaceWithText(jsonStr);

        setPresetFromFile(file);
        clearAllDirtyStates();
        emit("sessionSaved", new juce::DynamicObject());
    });
}

void StellarrBridge::handleSaveSessionQuiet()
{
    if (processor == nullptr) return;

    if (lastPresetFile.existsAsFile())
    {
        auto session = serialiseSession();
        auto jsonStr = juce::JSON::toString(session);
        lastPresetFile.replaceWithText(jsonStr);

        clearAllDirtyStates();
        emit("sessionSaved", new juce::DynamicObject());
    }
    else
    {
        handleSaveSession();
    }
}

void StellarrBridge::handleLoadSession()
{
    if (processor == nullptr) return;

    juce::MessageManager::callAsync([this]()
    {
        juce::FileChooser chooser("Load Preset", presetDirectory, "*.stellarr");

        if (!chooser.browseForFileToOpen()) return;

        auto file = chooser.getResult();
        auto jsonStr = file.loadFileAsString();
        auto session = juce::JSON::parse(jsonStr);
        restoreSession(session, [this, file](bool ok)
        {
            if (!ok) return;
            setPresetFromFile(file);
        });
    });
}

void StellarrBridge::handlePickPresetDirectory()
{
    juce::MessageManager::callAsync([this]()
    {
        juce::FileChooser chooser("Select Preset Directory");

        if (!chooser.browseForDirectory()) return;

        presetDirectory = chooser.getResult();
        currentPresetIndex = -1;
        handleGetPresetList();
        sendPresetList();
        persistPresetInfo();
    });
}

void StellarrBridge::handleRenamePreset(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    auto newName = obj->getProperty("name").toString().trim();
    if (index < 0 || index >= presetFiles.size() || newName.isEmpty()) return;

    auto oldFile = presetDirectory.getChildFile(presetFiles[index]);
    auto newFile = presetDirectory.getChildFile(newName + ".stellarr");

    if (newFile.existsAsFile() || !oldFile.existsAsFile()) return;

    if (oldFile.moveFileTo(newFile))
    {
        // Update tracking if this was the active preset
        if (index == currentPresetIndex)
            lastPresetFile = newFile;

        handleGetPresetList();

        // Find new index after re-sorting
        currentPresetIndex = -1;
        for (int i = 0; i < presetFiles.size(); ++i)
        {
            if (presetFiles[i] == newFile.getFileName())
            {
                currentPresetIndex = i;
                break;
            }
        }

        sendPresetList();
        persistPresetInfo();
    }
}

void StellarrBridge::handleDeletePreset(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    if (index < 0 || index >= presetFiles.size()) return;

    auto file = presetDirectory.getChildFile(presetFiles[index]);
    if (!file.existsAsFile()) return;

    if (file.deleteFile())
    {
        bool wasActive = (index == currentPresetIndex);

        handleGetPresetList();

        if (wasActive)
        {
            // Deleted the active preset — clear active state
            currentPresetIndex = -1;
            lastPresetFile = juce::File{};
        }
        else if (currentPresetIndex > index)
        {
            // Active preset shifted down
            currentPresetIndex--;
        }
        // else: active preset was before deleted one, index unchanged

        sendPresetList();
        persistPresetInfo();
    }
}

void StellarrBridge::handleGetPresetList()
{
    presetFiles.clear();

    if (presetDirectory.isDirectory())
    {
        for (auto& f : presetDirectory.findChildFiles(
                juce::File::findFiles, false, "*.stellarr"))
        {
            presetFiles.add(f.getFileName());
        }

        presetFiles.sort(true);
    }
}

void StellarrBridge::handleLoadPresetByIndex(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    if (index < 0 || index >= presetFiles.size()) return;

    // Clicking the already-active preset is a no-op — no spinner, no graph
    // rebuild, no disk read. The active-row highlight in the UI is driven by
    // currentPresetIndex which is already set, so nothing to update.
    if (index == currentPresetIndex) return;

    auto file = presetDirectory.getChildFile(presetFiles[index]);
    auto jsonStr = file.loadFileAsString();
    auto session = juce::JSON::parse(jsonStr);

    // Only update preset bookkeeping once the graph actually loads. If
    // restoreSession is rejected (lock held by a concurrent request, or
    // malformed session) or fails inside Phase 1/2, keep the previous active
    // preset so a subsequent quiet save persists the right file.
    restoreSession(session, [this, index, file](bool ok)
    {
        if (!ok) return;
        currentPresetIndex = index;
        setPresetFromFile(file);
    });
}

void StellarrBridge::sendPresetList()
{
    juce::Array<juce::var> files;
    for (auto& f : presetFiles)
        files.add(juce::var(f));

    auto* detail = new juce::DynamicObject();
    detail->setProperty("directory", presetDirectory.getFullPathName());
    detail->setProperty("files", files);
    detail->setProperty("currentIndex", currentPresetIndex);
    emit("presetListUpdated", detail);
}

// -- Grid dimensions ----------------------------------------------------------

void StellarrBridge::emitGridState()
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("columns", gridCols);
    detail->setProperty("rows", gridRows);
    emit("gridState", detail);
}

void StellarrBridge::handleSetGridSize(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto cols = obj->getProperty("columns");
    auto rows = obj->getProperty("rows");
    if (!(cols.isInt() || cols.isInt64() || cols.isDouble())) return;
    if (!(rows.isInt() || rows.isInt64() || rows.isDouble())) return;

    gridCols = juce::jmax(1, static_cast<int>(cols));
    gridRows = juce::jmax(1, static_cast<int>(rows));

    // Autosave so the new dimensions survive a restart, matching how block
    // edits persist.
    handleSaveSessionQuiet();
}
