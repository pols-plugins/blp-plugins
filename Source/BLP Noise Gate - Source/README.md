# BLP Noise Gate

A mono/stereo VST3 noise gate by BLP. Threshold spans -72 to -18 dB, Attack spans 1 to 150 ms, and Release spans 100 to 800 ms. The closed gate attenuates by 45 dB. The waveform illustrates the threshold setting.

## Windows x64 Release build

Use Visual Studio 2022 with Desktop development with C++, CMake 3.22 or newer, and an extracted JUCE 9.0.5 source tree. The commands below select the same JUCE version used for the delivered build.

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 "-DFETCHCONTENT_SOURCE_DIR_JUCE=C:/path/to/JUCE"
cmake --build build --config Release --target NoiseGate_VST3
```

The bundle is generated at `build/NoiseGate_artefacts/Release/VST3/BLP Noise Gate.vst3`. Copy the entire bundle directory to `C:\Program Files\Common Files\VST3` and rescan plugins in your DAW.
