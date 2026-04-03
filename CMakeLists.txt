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

juce_add_plugin(Stellarr
    COMPANY_NAME "Stellarr"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Stlr
    PLUGIN_CODE Stl1
    FORMATS Standalone VST3 AU
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
        engine/PluginManager.cpp
        engine/StellarrPlatform.mm
        engine/StellarrStandaloneApp.cpp
)

target_compile_definitions(Stellarr
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=1
        JUCE_MODAL_LOOPS_PERMITTED=1
    PRIVATE
        STELLARR_UI_DIR="${CMAKE_SOURCE_DIR}/ui/dist"
)

target_link_libraries(Stellarr
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

enable_testing()

set(TEST_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/engine)
set(TEST_LIBS Stellarr juce::juce_audio_utils juce::juce_gui_extra)

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
add_stellarr_test(SceneTest       engine/test/SceneTest.cpp)
add_stellarr_test(SessionTest      engine/test/SessionTest.cpp)
