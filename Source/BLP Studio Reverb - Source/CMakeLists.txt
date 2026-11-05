cmake_minimum_required(VERSION 3.22)
project(BLPStudioReverb VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(JUCE_SOURCE_DIR "" CACHE PATH "Local JUCE 8.0.6 checkout")
if(JUCE_SOURCE_DIR)
    add_subdirectory("${JUCE_SOURCE_DIR}" juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(StudioReverb
    COMPANY_NAME BLP
    PRODUCT_NAME "BLP Studio Reverb"
    DESCRIPTION "Algorithmic reverb recreating a studio space."
    PLUGIN_MANUFACTURER_CODE Blpa
    PLUGIN_CODE Bsrv
    FORMATS VST3
    VST3_CATEGORIES Fx Reverb)
target_sources(StudioReverb PRIVATE Plugin.cpp)
target_compile_definitions(StudioReverb PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(StudioReverb PRIVATE juce::juce_audio_processors juce::juce_recommended_config_flags)


if(WIN32)
    target_include_directories(StudioReverb PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(StudioReverb PRIVATE JUCE_USER_DEFINED_RC_FILE="Version.rc")
endif()
