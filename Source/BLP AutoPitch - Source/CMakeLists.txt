cmake_minimum_required(VERSION 3.22)
project(BLPAutoPitch VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BLP_JUCE_DIR "" CACHE PATH "Optional local JUCE 8.0.6 checkout")
if(BLP_JUCE_DIR)
    add_subdirectory("${BLP_JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(BLPAutoPitch COMPANY_NAME BLP BUNDLE_ID com.blp.autopitch
    PLUGIN_MANUFACTURER_CODE Blpa PLUGIN_CODE Bap1 PRODUCT_NAME "BLP AutoPitch"
    FORMATS VST3 VST3_CATEGORIES Fx "Pitch Shift" COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(BLPAutoPitch PRIVATE PluginProcessor.cpp PluginEditor.cpp)
target_compile_definitions(BLPAutoPitch PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPAutoPitch PRIVATE juce::juce_audio_processors juce::juce_recommended_config_flags)

