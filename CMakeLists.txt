cmake_minimum_required(VERSION 3.24)

# Read the canonical version from ui/package.json so the UI, the engine, and
# the app bundle's Info.plist never drift. JSON is parsed line-by-line via a
# regex rather than a CMake JSON module, which keeps the minimum CMake
# version at 3.24 (already required elsewhere).
file(READ "${CMAKE_SOURCE_DIR}/ui/package.json" STELLARR_PKG_JSON)
string(REGEX MATCH "\"version\"[ \t]*:[ \t]*\"([^\"]+)\"" _ "${STELLARR_PKG_JSON}")
set(STELLARR_VERSION "${CMAKE_MATCH_1}")
if(NOT STELLARR_VERSION)
  message(FATAL_ERROR "Could not parse version from ui/package.json")
endif()
message(STATUS "Stellarr version: ${STELLARR_VERSION}")

set(STELLARR_FLAVOUR "prod" CACHE STRING "Build flavour: prod | dev")
set_property(CACHE STELLARR_FLAVOUR PROPERTY STRINGS prod dev)

if(NOT STELLARR_FLAVOUR STREQUAL "prod" AND NOT STELLARR_FLAVOUR STREQUAL "dev")
  message(FATAL_ERROR "STELLARR_FLAVOUR must be prod or dev (got: ${STELLARR_FLAVOUR})")
endif()

if(STELLARR_FLAVOUR STREQUAL "dev")
  set(STELLARR_APP_NAME "Stellarr Dev")
  set(STELLARR_BUNDLE_ID "com.stellarr.stellarr.dev")
  set(STELLARR_FEED_URL  "https://stellarr.org/appcast-staging.xml")
else()
  set(STELLARR_APP_NAME "Stellarr")
  set(STELLARR_BUNDLE_ID "com.stellarr.stellarr")
  set(STELLARR_FEED_URL  "https://stellarr.org/appcast.xml")
endif()

message(STATUS "Stellarr flavour: ${STELLARR_FLAVOUR} (${STELLARR_APP_NAME} / ${STELLARR_BUNDLE_ID})")

project(Stellarr VERSION ${STELLARR_VERSION} LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 28e4d7b520d798f7f32b9fd6e1e022ee192feaf3
)
FetchContent_MakeAvailable(JUCE)

# Sparkle — macOS auto-update framework. Pinned release; bump deliberately.
set(SPARKLE_VERSION "2.9.1")
FetchContent_Declare(
    sparkle
    URL      https://github.com/sparkle-project/Sparkle/releases/download/${SPARKLE_VERSION}/Sparkle-${SPARKLE_VERSION}.tar.xz
    URL_HASH SHA256=c0dde519fd2a43ddfc6a1eb76aec284d7d888fe281414f9177de3164d98ba4c7
)
FetchContent_MakeAvailable(sparkle)

set(SPARKLE_FRAMEWORK_DIR "${sparkle_SOURCE_DIR}/Sparkle.framework")
if(NOT EXISTS "${SPARKLE_FRAMEWORK_DIR}")
    message(FATAL_ERROR "Sparkle.framework not found at ${SPARKLE_FRAMEWORK_DIR}")
endif()

# Sparkle EdDSA public keys baked into the app bundle's Info.plist so
# downloaded updates can be verified locally before install. The matching
# private keys live in the macOS Keychain (stellarr-prod / stellarr-dev
# accounts) and in GitHub Actions secrets SPARKLE_PRIVATE_KEY_{PROD,DEV}.
# Rotate via `make regen-sparkle-keys-prod` or `make regen-sparkle-keys-dev`.
set(STELLARR_SUPUBLIC_EDKEY_PROD "BU95nWg2poMoFLKgS29ukTwH0lChg79Id1xji3A50Pc=")
set(STELLARR_SUPUBLIC_EDKEY_DEV  "eJoHZUY/sMumVIF63iF+nilbcFXm2GpZUSwkYLWydbQ=")

if(STELLARR_FLAVOUR STREQUAL "dev")
    set(STELLARR_SUPUBLIC_EDKEY "${STELLARR_SUPUBLIC_EDKEY_DEV}")
else()
    set(STELLARR_SUPUBLIC_EDKEY "${STELLARR_SUPUBLIC_EDKEY_PROD}")
endif()

set(SENTRY_BACKEND "inproc" CACHE STRING "")
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "")
FetchContent_Declare(
    sentry
    GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
    GIT_TAG 0.13.7
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(sentry)

# JUCE's PLIST_TO_MERGE expects a full <plist><dict>...</dict></plist> XML
# document; it merges each <key>/<value> pair from our <dict> into the final
# Info.plist it generates for the target.
set(STELLARR_SPARKLE_PLIST_TEMPLATE [=[<?xml version="1.0" encoding="UTF-8"?>
<plist version="1.0">
<dict>
    <key>SUFeedURL</key>
    <string>@STELLARR_FEED_URL@</string>
    <key>SUPublicEDKey</key>
    <string>@STELLARR_SUPUBLIC_EDKEY@</string>
    <key>SUEnableAutomaticChecks</key>
    <true/>
    <key>SUAutomaticallyUpdate</key>
    <false/>
    <key>SUScheduledCheckInterval</key>
    <integer>86400</integer>
</dict>
</plist>
]=])
string(CONFIGURE "${STELLARR_SPARKLE_PLIST_TEMPLATE}" STELLARR_SPARKLE_PLIST @ONLY)

juce_add_plugin(Stellarr
    COMPANY_NAME "Stellarr"
    BUNDLE_ID "${STELLARR_BUNDLE_ID}"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Stellarr needs microphone access to process audio input from your audio interface."
    FORMATS Standalone
    PRODUCT_NAME "${STELLARR_APP_NAME}"
    ICON_BIG "${CMAKE_SOURCE_DIR}/assets/icon.png"
    ICON_SMALL "${CMAKE_SOURCE_DIR}/assets/icon.png"
    PLIST_TO_MERGE "${STELLARR_SPARKLE_PLIST}"
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
        engine/bridge/UpdateHandler.cpp
        engine/blocks/Block.cpp
        engine/blocks/PluginBlock.cpp
        engine/blocks/InputBlock.cpp
        engine/utils/ToneGenerator.cpp
        engine/MidiMapper.cpp
        engine/PluginManager.cpp
        engine/StellarrPlatform.mm
        engine/StellarrStandaloneApp.cpp
        engine/UpdaterShim.mm
)

# UpdaterShim uses Objective-C++ blocks captured from Sparkle's user-driver
# protocol. Under MRC, copying a @property block pointer into a local and
# then niling the property frees the block before we invoke it. Enabling ARC
# just for this file gives us correct block lifetime semantics without
# touching the rest of the codebase (JUCE's .mm files remain MRC).
set_source_files_properties(
    engine/UpdaterShim.mm
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
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

if(STELLARR_FLAVOUR STREQUAL "dev")
    target_compile_definitions(Stellarr PRIVATE STELLARR_IS_DEV=1)
else()
    target_compile_definitions(Stellarr PRIVATE STELLARR_IS_DEV=0)
endif()

# Sparkle framework: add search path so UpdaterShim.mm can #import <Sparkle/*>
# on the shared-code target, link against the framework on the Standalone
# target, and copy the framework into the .app bundle after link so the
# loader finds it at @rpath/Sparkle.framework.
target_compile_options(Stellarr PRIVATE "-F${sparkle_SOURCE_DIR}")
target_link_libraries(Stellarr_Standalone PRIVATE "-F${sparkle_SOURCE_DIR}" "-framework Sparkle")
target_link_options(Stellarr_Standalone PRIVATE "-Wl,-rpath,@executable_path/../Frameworks")

add_custom_command(TARGET Stellarr_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:Stellarr_Standalone>/Contents/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SPARKLE_FRAMEWORK_DIR}"
            "$<TARGET_BUNDLE_DIR:Stellarr_Standalone>/Contents/Frameworks/Sparkle.framework"
    COMMENT "Embedding Sparkle.framework into ${STELLARR_APP_NAME}.app"
)

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
        # The shared-code library pulls in UpdaterShim.mm, which references
        # Sparkle symbols. Test executables only need the symbols to resolve
        # at link time — they never actually exercise the update path — so
        # passing the framework flag is enough; no rpath or bundle copy.
        target_link_libraries(${NAME} PRIVATE ${TEST_LIBS} "-F${sparkle_SOURCE_DIR}" "-framework Sparkle")
        target_link_options(${NAME} PRIVATE "-Wl,-rpath,${sparkle_SOURCE_DIR}")
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
    add_stellarr_test(LoudnessMeterTest  engine/test/LoudnessMeterTest.cpp)
endif()
