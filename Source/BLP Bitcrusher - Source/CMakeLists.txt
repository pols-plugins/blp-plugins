cmake_minimum_required(VERSION 3.22)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
project(BLPBitcrusher VERSION 1.0.2 LANGUAGES C CXX)

set(BLP_JUCE_SOURCE "" CACHE PATH "Path to a local JUCE 8.0.12 checkout")
if(BLP_JUCE_SOURCE)
    add_subdirectory("${BLP_JUCE_SOURCE}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        URL https://codeload.github.com/juce-framework/JUCE/zip/refs/tags/8.0.12)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BLPBitcrusher
    COMPANY_NAME "BLP"
    BUNDLE_ID com.blp.bitcrusher
    PLUGIN_MANUFACTURER_CODE BlpA
    PLUGIN_CODE Bc7R
    PRODUCT_NAME "BLP Bitcrusher"
    VST3_CATEGORIES Fx Distortion
    FORMATS VST3)

target_sources(BLPBitcrusher PRIVATE PluginProcessor.cpp PluginEditor.cpp)
target_compile_features(BLPBitcrusher PRIVATE cxx_std_17)
target_compile_definitions(BLPBitcrusher PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPBitcrusher PRIVATE juce::juce_audio_processors
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)




