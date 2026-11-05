# BLP Auto Pan 1.0.0

## Install on Windows x64

Extract the Windows archive. Copy the entire **BLP Auto Pan.vst3** folder to
**C:/Program Files/Common Files/VST3**, then rescan VST3 effects in your DAW.
Insert BLP Auto Pan on a track with a stereo output.

## Controls

- **Rate** sets the time for a full left-to-right-to-left cycle. Click **beat** or **Hz** to switch modes. Each mode remembers its rate. Beat mode follows the DAW timeline during playback and uses 120 BPM if the host supplies no tempo. While stopped, it runs at the available tempo so you can audition the effect.
- **Spread** controls how far the signal moves. At 0%, stereo audio passes through unchanged. A mono input uses equal-power routing to the stereo output, including a centered dry signal.
- **Bypass** switches to dry audio and dims the controls. Double-click either knob to restore its default.

## Build from source

Use CMake 3.22 or newer and Visual Studio 2022 with the Desktop development with C++ workload.
Run these commands from this source folder.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPAutoPan_VST3
```

CMake fetches JUCE 8.0.8. To build with a local copy of that version, add
`-DJUCE_DIR="C:/path/to/JUCE"` to the first command.
The bundle appears under `build/BLPAutoPan_artefacts/Release/VST3`.
