cmake_minimum_required(VERSION 3.22)
project(BLPDriver VERSION 1.0.0 LANGUAGES C CXX)

set(JUCE_DIR "" CACHE PATH "Path to a JUCE 8.0.6 source tree")
if(JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                              GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BLPDriver
    COMPANY_NAME BLP
    BUNDLE_ID audio.blp.driver
    PLUGIN_MANUFACTURER_CODE BlpA
    PLUGIN_CODE BDrv
    FORMATS VST3
    VST3_CATEGORIES Fx Distortion
    PRODUCT_NAME "BLP Driver")

target_sources(BLPDriver PRIVATE PluginProcessor.cpp PluginEditor.cpp)
target_compile_features(BLPDriver PRIVATE cxx_std_17)
target_compile_definitions(BLPDriver PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPDriver PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
