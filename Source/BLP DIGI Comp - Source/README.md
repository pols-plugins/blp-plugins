# BLP DIGI Comp 1.0.0

BLP DIGI Comp is a Windows x64 VST3 dynamics effect for mono and stereo tracks. It uses feed-forward peak detection, a linked stereo gain envelope, and a quadratic soft knee. It reports zero samples of latency.

## Install

1. Extract the Windows ZIP.
2. Copy the entire **BLP DIGI Comp.vst3** folder into **C:\Program Files\Common Files\VST3\**. Keep the folder structure intact.
3. Rescan VST3 effects in your DAW and look for **BLP DIGI Comp** under **BLP**.

Drag a knob vertically to adjust it. Hold Shift while dragging for finer adjustment and double-click to reset. The Bypass control lights up when compression is bypassed. All six controls can be automated and saved with the host project.

## Build from source

Install Visual Studio 2022 with the Desktop development with C++ workload and a Windows SDK. Install CMake 3.22 or later and Git. Extract the source ZIP, open PowerShell in the project folder, and run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPDIGIComp_VST3
```

CMake fetches JUCE 8.0.6. For an offline build, append `-DJUCE_DIR=C:/path/to/JUCE` to the configure command with a local JUCE 8.0.6 checkout.

The completed bundle is at `build/BLPDIGIComp_artefacts/Release/VST3/BLP DIGI Comp.vst3`.

If MSVC reports C1902 while building JUCE's helper, set `$env:_CL_ = "/Z7"` in the same PowerShell session and rerun the commands.
