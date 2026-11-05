# BLP Chorus Ensemble

Windows x64 VST3 modulation effect by BLP. Three interpolated chorus voices per output channel use different delay times and LFO phases. Bass and Treble shape the wet signal, and the plug-in smooths control and bypass changes over 30 ms. Mono, stereo, and mono-to-stereo layouts are supported.

## Install

1. Extract the Windows ZIP. Copy the entire **BLP Chorus Ensemble.vst3** folder into **C:\Program Files\Common Files\VST3**. Windows may prompt for administrator approval.
2. Rescan VST3 effects in your DAW's plug-in manager and insert **BLP Chorus Ensemble** on an audio track or mixer channel. In FL Studio, use Manage plugins and Find installed plugins. In Cakewalk, use its VST scan. In REAPER, use Preferences > Plug-ins > VST and Re-scan.

Drag a knob or scroll over it to adjust. Hold Shift while dragging for finer control, or double-click to restore its default. The Bypass checkbox fades back to the dry signal.

## Build from source

Install Visual Studio 2022 with Desktop development with C++, the Windows SDK, CMake 3.22 or newer, and Git. Extract the source ZIP, open PowerShell in the source folder and run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPChorus_VST3
```

CMake downloads JUCE 8.0.6 on the first configure. To use an existing checkout, add `"-DJUCE_DIR=C:/path/to/JUCE"` to the configure command. The compiled bundle appears under `build/BLPChorus_artefacts/Release/VST3/`. The generated Visual Studio solution can also build the BLPChorus_VST3 target in Release x64.

