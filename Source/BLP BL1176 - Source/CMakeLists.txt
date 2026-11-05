cmake_minimum_required(VERSION 3.22)
project(BLP1176 VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.8 GIT_SHALLOW TRUE GIT_SUBMODULES "")
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(BLP1176 COMPANY_NAME BLP PRODUCT_NAME "BLP BL1176"
    PLUGIN_MANUFACTURER_CODE Blpa PLUGIN_CODE B117 FORMATS VST3
    VST3_CATEGORIES Fx Dynamics COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(BLP1176 PRIVATE Plugin.cpp)
target_compile_definitions(BLP1176 PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BLP1176 PRIVATE juce::juce_audio_processors juce::juce_recommended_config_flags)
