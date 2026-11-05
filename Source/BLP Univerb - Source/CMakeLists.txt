cmake_minimum_required(VERSION 3.22)
project(BLPUniverb VERSION 1.0.0 LANGUAGES C CXX)
set(BLP_JUCE_SOURCE "" CACHE PATH "Path to JUCE 8.0.12")
if(BLP_JUCE_SOURCE)
    add_subdirectory("${BLP_JUCE_SOURCE}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE URL https://codeload.github.com/juce-framework/JUCE/zip/refs/tags/8.0.12)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(BLPUniverb
    COMPANY_NAME BLP
    BUNDLE_ID com.blp.univerb
    PLUGIN_MANUFACTURER_CODE BlpA
    PLUGIN_CODE UnvR
    PRODUCT_NAME "BLP Univerb"
    VST3_CATEGORIES Fx Reverb
    FORMATS VST3)
target_sources(BLPUniverb PRIVATE Plugin.cpp)
target_compile_features(BLPUniverb PRIVATE cxx_std_17)
target_compile_definitions(BLPUniverb PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPUniverb PRIVATE juce::juce_audio_processors PUBLIC juce::juce_recommended_config_flags)

