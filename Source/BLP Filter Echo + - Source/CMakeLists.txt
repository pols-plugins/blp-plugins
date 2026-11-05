cmake_minimum_required(VERSION 3.22)
project(BLPFilterEchoPlus VERSION 1.0.0 LANGUAGES C CXX)
set(JUCE_DIR "" CACHE PATH "Optional path to a JUCE source checkout")
if(JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/juce")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE URL https://codeload.github.com/juce-framework/JUCE/zip/refs/tags/8.0.12)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(FilterEcho COMPANY_NAME BLP BUNDLE_ID com.blp.filterechoplus
    PLUGIN_MANUFACTURER_CODE Blpe PLUGIN_CODE Fecp
    FORMATS VST3 PRODUCT_NAME "BLP Filter Echo +" VST3_CATEGORIES Fx Delay
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(FilterEcho PRIVATE PluginProcessor.cpp PluginEditor.cpp)
target_compile_features(FilterEcho PRIVATE cxx_std_17)
target_compile_definitions(FilterEcho PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(FilterEcho PRIVATE juce::juce_audio_processors juce::juce_recommended_config_flags)

