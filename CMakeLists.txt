cmake_minimum_required(VERSION 3.25)

# Build-system helpers live in /cmake (the sudara/cmake-includes submodule from Pamplejuce)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PamplejuceVersion)   # reads VERSION into CURRENT_VERSION
include(CPM)                 # dependency fetching (Catch2)
include(PamplejuceMacOS)     # deployment target + universal binaries on CI
include(JUCEDefaults)

# Internal target name (no spaces). Changing it later renames build artefacts only.
set(PROJECT_NAME "Solfa")

# Name shown in DAWs.
set(PRODUCT_NAME "Solfa")
set(COMPANY_NAME "Solfa Audio")
set(BUNDLE_ID "com.solfaaudio.solfa")

# IMPORTANT: once anyone saves a DAW session with Solfa in it, never change
# PLUGIN_MANUFACTURER_CODE, PLUGIN_CODE or BUNDLE_ID: hosts use them to find the plugin.

# Formats. AU is macOS-only; AAX is macOS/Windows only (JUCE bundles the AAX SDK).
# CLAP is added below via clap-juce-extensions. VST2 is not available to new developers.
set(FORMATS Standalone VST3)
if (APPLE)
    list(APPEND FORMATS AU AAX)
elseif (WIN32)
    list(APPEND FORMATS AAX)
endif ()

set(CMAKE_POLICY_DEFAULT_CMP0141 NEW CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

include(PamplejuceLog)
include(PamplejuceFunctions)

# JUCE 9.0.6, pinned as a git submodule
add_subdirectory(JUCE)
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)

juce_add_plugin("${PROJECT_NAME}"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"
    PRODUCT_NAME "${PRODUCT_NAME}"
    FORMATS "${FORMATS}"

    # Four-character IDs. Manufacturer code must start with an uppercase letter (AU rule).
    PLUGIN_MANUFACTURER_CODE Slfa
    PLUGIN_CODE Sfeq

    # An audio effect: stereo/mono in -> out, no MIDI.
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    # How hosts categorise the plugin
    VST3_CATEGORIES Fx EQ
    AU_MAIN_TYPE kAudioUnitType_Effect
    AAX_CATEGORY AAX_ePlugInCategory_EQ

    # Copies plugins into the user's plug-in folders after building (handy on your Mac)
    COPY_PLUGIN_AFTER_BUILD TRUE)

clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}"
    CLAP_ID "${BUNDLE_ID}"
    CLAP_FEATURES audio-effect equalizer stereo mono)

# Plugin code shared by the plugin targets and the Tests target (avoids ODR violations)
add_library(SharedCode INTERFACE)
pamplejuce_shared_code_defaults()   # C++23, fast math off for tests, etc.

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

pamplejuce_xcode_prettify()

target_compile_definitions(SharedCode
    INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"
    PRODUCT_NAME_WITHOUT_VERSION="${PRODUCT_NAME}")

target_link_libraries(SharedCode
    INTERFACE
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

pamplejuce_add_tests()
pamplejuce_add_benchmarks()

include(GitHubENV)   # exports PRODUCT_NAME etc. for CI
