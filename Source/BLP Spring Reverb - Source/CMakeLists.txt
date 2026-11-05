cmake_minimum_required(VERSION 3.22)
project(BLPSpringReverb VERSION 1.0.0 LANGUAGES CXX)

set(JUCE_DIR "" CACHE PATH "Local JUCE source tree (optional)")
if(JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(JUCE
        URL https://codeload.github.com/juce-framework/JUCE/zip/refs/tags/8.0.8)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BLPSpringReverb
    COMPANY_NAME BLP
    BUNDLE_ID com.blp.springreverb
    PLUGIN_MANUFACTURER_CODE Blpp
    PLUGIN_CODE Sprv
    PRODUCT_NAME "BLP Spring Reverb"
    VST3_CATEGORIES Fx Reverb
    FORMATS VST3
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(BLPSpringReverb PRIVATE Plugin.cpp)
target_compile_features(BLPSpringReverb PRIVATE cxx_std_17)
target_compile_definitions(BLPSpringReverb PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLPSpringReverb PRIVATE
    juce::juce_audio_processors juce::juce_recommended_config_flags)

