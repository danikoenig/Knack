cmake_minimum_required(VERSION 3.22)

project(Knack VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: lokaler Pfad (-DJUCE_PATH=...) oder automatischer Download
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Knack
    COMPANY_NAME "Dani Koenig"
    BUNDLE_ID "ch.danikoenig.knack"
    PLUGIN_MANUFACTURER_CODE Dkng
    PLUGIN_CODE Knk1
    FORMATS VST3 AU Standalone
    AU_MAIN_TYPE kAudioUnitType_Effect
    PRODUCT_NAME "Knack"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Distortion)

target_sources(Knack PRIVATE
    Source/Engine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Look.cpp)

target_compile_definitions(Knack PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Knack
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optionaler Test (nur lokal): -DKNACK_TEST_SRC=tests/EngineTest.cpp
if(KNACK_TEST_SRC)
    add_executable(KnackTest ${KNACK_TEST_SRC})
    target_link_libraries(KnackTest PRIVATE Knack)
    target_include_directories(KnackTest PRIVATE $<TARGET_PROPERTY:Knack,INCLUDE_DIRECTORIES>)
    target_compile_definitions(KnackTest PRIVATE $<TARGET_PROPERTY:Knack,COMPILE_DEFINITIONS>)
endif()
