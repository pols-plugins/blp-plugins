# BLP Stereowise

Windows x64 stereo imaging effect by BLP. Spread scales both existing stereo differences and a phase-shifted contribution from centered audio. Low and High control two complementary bands around 500 Hz. The mono sum is preserved. A mono source can widen when routed through a stereo track or bus. A single-channel plugin instance passes through, and parameter and bypass transitions ramp over 20 ms. A peak guard limits the added side on loud samples. The processor adds zero samples of latency.

## Install

Extract the download and copy the entire `BLP Stereowise.vst3` bundle to `C:\Program Files\Common Files\VST3`. Rescan VST3 plug-ins in your DAW and insert BLP Stereowise as an audio effect on a stereo track. For hosts that support custom VST3 folders, you can also point the host at the folder containing the bundle.

Drag vertically or scroll to adjust a knob. Double-click the knob face to reset it. Right-click the knob or click its numerical value to type a setting. The orange bypass check mark means the effect is bypassed.

## Build

Install Visual Studio 2022 with Desktop development with C++, a Windows SDK, CMake 3.22 or newer, and Git. Open PowerShell in the folder containing this README.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPStereowise_VST3 --parallel
```

CMake fetches JUCE 8.0.6 by default. For an offline build, add `-DJUCE_SOURCE_DIR=C:/path/to/JUCE` to the first command. The bundle appears under `build/BLPStereowise_artefacts/Release/VST3`. See [JUCE's licensing terms](https://juce.com/legal/juce-8-licence/) before distributing a build.


