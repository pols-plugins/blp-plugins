# BLP Spring Reverb

BLP Spring Reverb is a 64-bit Windows VST3 audio effect with matching mono and stereo buses. Amount defaults to 20%. Drag the Amount line up to increase the effect or down to decrease it. Double-click the Amount control to reset it. American is the brighter voicing and UK is the darker voicing. An illuminated Bypass button indicates that the effect is bypassed. All three parameters can be automated and are saved with the DAW project.

## Install on Windows 10 or 11

Extract the Windows ZIP and copy the entire **BLP Spring Reverb.vst3** folder into **C:\Program Files\Common Files\VST3**. Keep its internal folder structure intact. Rescan VST3 plug-ins in your DAW, then look for **BLP Spring Reverb** under BLP or Reverb.

## Build from source

Install Visual Studio 2022 with the Desktop development with C++ workload and a Windows SDK. Install CMake 3.22 or newer. In PowerShell, open the extracted source directory and run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPSpringReverb_VST3 -- /m:1 /nr:false
```

CMake downloads JUCE 8.0.8 on the first build. To use an existing JUCE 8.0.8 source tree, add `-DJUCE_DIR="C:/path/to/JUCE"` to the configure command. The build produces the bundle at `build/BLPSpringReverb_artefacts/Release/VST3/BLP Spring Reverb.vst3`.

## Signal path

Each voicing uses three coupled feedback delays with cascaded allpass filters for dispersion, slow delay modulation for flutter, and damping in the feedback loops. Stereo uses slightly different delay lengths on each side. Amount and Bypass ramp the reverb mix over 35 ms, and Type crossfades the two tanks over 80 ms. The dry signal remains present throughout the Amount range.
