cmake_minimum_required(VERSION 3.22)
project(AutoWave VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# OJO: el nombre interno "VocalLock" NO se cambia a propósito: el archivo de GitHub Actions
# busca la carpeta VocalLock_artefacts. El nombre visible del plugin es PRODUCT_NAME.
juce_add_plugin(VocalLock
    COMPANY_NAME "KIDZEBA"
    BUNDLE_ID com.kidzeba.autowave
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Kzba
    PLUGIN_CODE Awav
    FORMATS VST3 Standalone
    PRODUCT_NAME "AUTO WAVE")

target_sources(VocalLock PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp)

target_compile_definitions(VocalLock PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VocalLock
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
