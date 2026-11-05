cmake_minimum_required(VERSION 3.24)
project(BLPPitchShifter VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(JUCE URL https://codeload.github.com/juce-framework/JUCE/zip/refs/tags/8.0.6 DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(JUCE)
if(TARGET juceaide AND NOT TARGET juce::juceaide)
  add_executable(juce::juceaide ALIAS juceaide)
endif()
FetchContent_Declare(stretch GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git GIT_TAG 88061fd4eda56a2353c5d65c74aa3f4ccf76f9da)
FetchContent_MakeAvailable(stretch)
juce_add_plugin(BLPPitchShifter
  COMPANY_NAME BLP PLUGIN_MANUFACTURER_CODE Blpa PLUGIN_CODE Blps
  PRODUCT_NAME "BLP Pitch Shifter" BUNDLE_ID audio.blp.pitchshifter
  FORMATS VST3 VST3_CATEGORIES Fx PitchShift)
target_sources(BLPPitchShifter PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(BLPPitchShifter PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPPitchShifter PRIVATE juce::juce_audio_utils signalsmith-stretch juce::juce_recommended_config_flags)


