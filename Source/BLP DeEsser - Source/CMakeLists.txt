cmake_minimum_required(VERSION 3.22)
project(DeEsser VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "JUCE source")
add_subdirectory("${JUCE_DIR}" JUCE)
juce_add_plugin(DeEsser COMPANY_NAME "BLP" BUNDLE_ID "audio.blp.graphiceq" PLUGIN_MANUFACTURER_CODE Blpx PLUGIN_CODE Bge1 FORMATS VST3 PRODUCT_NAME "BLP DeEsser" VST3_CATEGORIES Fx Dynamics COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(DeEsser PRIVATE Source/Plugin.cpp)
target_compile_definitions(DeEsser PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(DeEsser PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
juce_add_console_app(DeEsserTests PRODUCT_NAME "DeEsserTests")
target_sources(DeEsserTests PRIVATE Source/Test.cpp)
target_compile_definitions(DeEsserTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(DeEsserTests PRIVATE juce::juce_audio_utils)



