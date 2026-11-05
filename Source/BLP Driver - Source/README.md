# BLP Driver

BLP Driver is a Windows x64 VST3 effect by BLP. It runs on mono and stereo tracks and reports zero samples of latency.

## Install

Copy the entire **BLP Driver.vst3** folder into the Windows VST3 folder, normally
`C:\Program Files\Common Files\VST3`. Rescan plug-ins in your DAW and load **BLP Driver**
as an audio effect.

## Controls

Drag the Drive control, Tone slider, or Volume slider vertically to adjust its value.

| Control | Default | Range |
| --- | --- | --- |
| Drive | 3.5 | 0 to 10 |
| Tone | 1800 Hz | 250 to 16000 Hz |
| Volume | 4.5 dB | 0 to 10 dB |

Drive blends in a soft-clipping waveshaper and increases the gain into it.
Tone applies a low-pass filter after the waveshaper and uses a logarithmic frequency scale.
Volume applies output gain after Tone. The synthetic waveform uses the same
processing stages and runs even when transport is stopped. Bypass crossfades
to the original signal over 10 ms and retains all settings. All controls
support host automation and state recall.

## Build on Windows

Install Visual Studio 2022 with **Desktop development with C++**, a Windows
SDK, CMake 3.22 or newer, and Git. Run these commands from the source directory.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPDriver_VST3 --parallel 4
```

CMake fetches JUCE 8.0.6. To build from an existing JUCE 8.0.6 source checkout,
add `-DJUCE_DIR="C:/path/to/JUCE"` to the configure command.

The build creates `build/BLPDriver_artefacts/Release/VST3/BLP Driver.vst3`.
The bundle identifier is `audio.blp.driver`. JUCE uses manufacturer code
`BlpA` and plug-in code `BDrv` to generate the VST3 class identifiers.
