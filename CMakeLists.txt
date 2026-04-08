cmake_minimum_required(VERSION 3.24)
project(Stellarr VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG develop
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

set(SENTRY_BACKEND "inproc" CACHE STRING "")
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "")
FetchContent_Declare(
    sentry
    GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
    GIT_TAG 0.13.4
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(sentry)

juce_add_plugin(Stellarr
    COMPANY_NAME "Stellarr"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Stellarr needs microphone access to process audio input from your audio interface."
    FORMATS Standalone
    PRODUCT_NAME "Stellarr"
    ICON_BIG "${CMAKE_SOURCE_DIR}/assets/icon.png"
    ICON_SMALL "${CMAKE_SOURCE_DIR}/assets/icon.png"
)

target_sources(Stellarr
    PRIVATE
        engine/StellarrProcessor.cpp
        engine/StellarrEditor.cpp
        engine/StellarrBridge.cpp
        engine/bridge/GraphHandler.cpp
        engine/bridge/ParamHandler.cpp
        engine/bridge/SceneHandler.cpp
        engine/bridge/PresetHandler.cpp
        engine/bridge/MidiHandler.cpp
        engine/blocks/Block.cpp
        engine/blocks/PluginBlock.cpp
        engine/blocks/InputBlock.cpp
        engine/utils/ToneGenerator.cpp
        engine/MidiMapper.cpp
        engine/PluginManager.cpp
        engine/StellarrPlatform.mm
        engine/StellarrStandaloneApp.cpp
)

target_compile_definitions(Stellarr
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=1
        JUCE_MODAL_LOOPS_PERMITTED=1
    PRIVATE
        STELLARR_UI_DIR="${CMAKE_SOURCE_DIR}/ui/dist"
        STELLARR_SAMPLES_DIR="${CMAKE_SOURCE_DIR}/assets/samples/ui/public/inputs"
)

if(DEFINED STELLARR_SENTRY_DSN)
    target_compile_definitions(Stellarr PRIVATE STELLARR_SENTRY_DSN="${STELLARR_SENTRY_DSN}")
endif()

target_link_libraries(Stellarr
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        sentry::sentry
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(BUILD_TESTING)
    enable_testing()

    set(TEST_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/engine)
    set(TEST_LIBS Stellarr juce::juce_audio_utils juce::juce_gui_extra sentry::sentry)

    function(add_stellarr_test NAME SOURCE)
        add_executable(${NAME} ${SOURCE})
        target_include_directories(${NAME} PRIVATE ${TEST_INCLUDE_DIR})
        target_link_libraries(${NAME} PRIVATE ${TEST_LIBS})
        add_test(NAME ${NAME} COMMAND ${NAME})
    endfunction()

    add_stellarr_test(PassThroughTest  engine/test/PassThroughTest.cpp)
    add_stellarr_test(GraphBasicTest   engine/test/GraphBasicTest.cpp)
    add_stellarr_test(GraphRoutingTest engine/test/GraphRoutingTest.cpp)
    add_stellarr_test(GraphEdgeTest    engine/test/GraphEdgeTest.cpp)
    add_stellarr_test(BlockTest        engine/test/BlockTest.cpp)
    add_stellarr_test(TunerTest       engine/test/TunerTest.cpp)
    add_stellarr_test(MidiMapperTest  engine/test/MidiMapperTest.cpp)
    add_stellarr_test(SceneTest       engine/test/SceneTest.cpp)
    add_stellarr_test(SessionTest      engine/test/SessionTest.cpp)
    add_stellarr_test(PresetFileTest   engine/test/PresetFileTest.cpp)
    add_stellarr_test(CopyPasteTest   engine/test/CopyPasteTest.cpp)
    add_stellarr_test(PeakLevelTest      engine/test/PeakLevelTest.cpp)
    add_stellarr_test(PresetSwitchTest   engine/test/PresetSwitchTest.cpp)
    add_stellarr_test(TelemetryTest      engine/test/TelemetryTest.cpp)
endif()
