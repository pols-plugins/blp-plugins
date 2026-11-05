cmake_minimum_required(VERSION 3.22)
project(BLPStereowise VERSION 1.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_SOURCE_DIR "" CACHE PATH "Optional path to a JUCE source checkout")
if(JUCE_SOURCE_DIR)
    add_subdirectory("${JUCE_SOURCE_DIR}" juce)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                         GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BLPStereowise
    COMPANY_NAME BLP
    BUNDLE_ID audio.blp.stereowise
    PLUGIN_MANUFACTURER_CODE Blpa
    PLUGIN_CODE Swid
    PRODUCT_NAME "BLP Stereowise"
    FORMATS VST3
    VST3_CATEGORIES Fx Spatial
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(BLPStereowise PRIVATE PluginProcessor.cpp PluginEditor.cpp StereoWidthProcessor.cpp)
target_compile_definitions(BLPStereowise PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPStereowise PRIVATE juce::juce_audio_processors juce::juce_recommended_config_flags)
if(MSVC)
    target_compile_options(BLPStereowise PRIVATE /MP)
endif()


