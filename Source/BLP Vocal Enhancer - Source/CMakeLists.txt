cmake_minimum_required(VERSION 3.22)
project(BLPVocalEnhancer VERSION 1.0.0 LANGUAGES C CXX)

set(BLP_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout")
if(BLP_JUCE_DIR)
    add_subdirectory("${BLP_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BLPVocalEnhancer
    COMPANY_NAME BLP
    BUNDLE_ID com.blp.vocalenhancer
    PLUGIN_MANUFACTURER_CODE BlpA
    PLUGIN_CODE BvE1
    PRODUCT_NAME "BLP Vocal Enhancer"
    VST3_CATEGORIES Fx EQ
    FORMATS VST3)

target_sources(BLPVocalEnhancer PRIVATE
    PluginProcessor.cpp PluginEditor.cpp VocalEnhancerDSP.cpp)
target_compile_features(BLPVocalEnhancer PRIVATE cxx_std_17)
target_compile_definitions(BLPVocalEnhancer PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPVocalEnhancer PRIVATE juce::juce_audio_processors juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
