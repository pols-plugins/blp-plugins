cmake_minimum_required(VERSION 3.22)
project(BLPSTChorus VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JUCE_SOURCE_DIR "" CACHE PATH "Optional local JUCE source checkout")
if(JUCE_SOURCE_DIR)
    add_subdirectory("${JUCE_SOURCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                              GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BLPSTChorus
    COMPANY_NAME BLP
    BUNDLE_ID audio.blp.stchorus
    PLUGIN_MANUFACTURER_CODE Blpa
    PLUGIN_CODE Bpsc
    PRODUCT_NAME "BLP ST Chorus"
    FORMATS VST3
    VST3_CATEGORIES Fx Modulation)
target_sources(BLPSTChorus PRIVATE PluginProcessor.cpp PluginEditor.cpp)
target_compile_definitions(BLPSTChorus PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
                                              JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPSTChorus PRIVATE juce::juce_audio_processors
    PUBLIC juce::juce_recommended_config_flags)
