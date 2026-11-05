# BLP AutoPitch 1.0.0

BLP AutoPitch is a Windows x64 VST3 effect for a single vocal melody. It accepts mono or stereo tracks. On stereo tracks it detects pitch from the summed signal and applies the same correction to both channels.

## Install

Extract the Windows ZIP. Copy the entire **BLP AutoPitch.vst3** folder to `C:\Program Files\Common Files\VST3`, or to a VST3 folder your DAW scans. Rescan VST3 plug-ins, then insert **BLP AutoPitch** as an effect on a vocal track.

## Use

- Hover over or drag the knob to see its value. From 0% to 99%, it shows the percentage. At 100% it reads **Heaviest**. At 0%, the original pitch is preserved.
- **Scale** and the note buttons select the allowed notes. Chromatic uses all semitones. Its root is stored for when you switch to a keyed scale.
- **Bypass** crossfades to latency-matched dry audio. The plugin reports approximately 6 ms of latency for host compensation. This delay is also present at 0% tuning.

The DAW saves Tuning, Scale, Root Note and Bypass with the session. Each supports automation.

## Build on Windows

Install Visual Studio 2022 with **Desktop development with C++**, an x64 Windows SDK, and the CMake tools component. Install Git. Open the **Developer PowerShell for VS 2022** configured for x64 and change to this source directory. Run:

```powershell
$env:_CL_ = '/Z7'
$env:CMAKE_BUILD_PARALLEL_LEVEL = '4'
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target BLPAutoPitch_VST3
```

The first configure downloads JUCE 8.0.6. For a local JUCE 8.0.6 checkout, add `-DBLP_JUCE_DIR="C:/path/to/JUCE"` to the configure command. The release bundle is at `build/BLPAutoPitch_artefacts/Release/VST3/BLP AutoPitch.vst3`.

The processor uses a YIN-style detector and pitch-synchronous overlap-add. Grain spacing shifts the pitch while individual grains read at unity speed, which helps preserve the vocal envelope.

JUCE is a separate dependency governed by its license. Review the [JUCE licensing terms](https://juce.com/legal/juce-8-licence/) before distributing your builds.






