# BLP D-Delay+

Stereo VST3 delay by BLP. The supplied Release build targets 64-bit Windows.

## Install

Extract the release archive and copy the entire **BLP D-Delay+.vst3** folder into
`C:\Program Files\Common Files\VST3` or a VST3 directory configured in your host.
Rescan plug-ins and insert BLP D-Delay+ on a stereo track.

## Build

Install Visual Studio 2022 with Desktop development with C++, the Windows SDK,
CMake 3.22 or newer, and Git. From this source directory run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPDelay_VST3 --parallel
```

CMake fetches JUCE 8.0.8 on the first configure. To use a local JUCE 8.0.8
checkout, add `-DFETCHCONTENT_SOURCE_DIR_JUCE=C:/path/to/JUCE` to the first command.
Find the bundle at `build/BLPDelay_artefacts/Release/VST3/BLP D-Delay+.vst3`.

## Controls

Drag values vertically and double-click to reset. In BEAT mode the value opens
the note menu. In MS mode drag the same indicator or click the displayed value to type milliseconds.
The host stores both time settings and the mode independently.

The delay supports a 32-second center time for tempo sync. At maximum Spread the
taps move up to 25% earlier and later, symmetrically, with smaller offsets near
the buffer limits. At 100% feedback, the loop applies unity feedback gain.
Repeat decay remains frequency dependent because the filters run on each pass.



