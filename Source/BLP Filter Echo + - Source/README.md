# BLP Filter Echo +

A mono/stereo tempo-synchronized VST3 delay for 64-bit Windows. Beat and ms
store separate timing values. Drag or scroll over Feedback, Cutoff, Mix,
or the manual time value. Double-click any of these controls to reset it.

The delay uses 120 BPM until it receives a valid tempo and then retains the
last valid tempo if the host stops reporting it. Feedback tops out at 0.95
internal gain. Beat delays are capped at 12 seconds.

## Build a Windows x64 release

Install Visual Studio 2022 with Desktop development with C++, a Windows SDK,
and CMake 3.22 or later. Open PowerShell in this source directory.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target FilterEcho_VST3 --parallel
```

CMake fetches JUCE 8.0.12. To build offline, append
`-DJUCE_DIR="C:/path/to/JUCE"` to the configure command and point it at a
stock JUCE 8.0.12 source checkout.

The complete bundle is written to
`build/FilterEcho_artefacts/Release/VST3/BLP Filter Echo +.vst3`.
Copy the entire bundle folder into
`C:/Program Files/Common Files/VST3` and rescan plug-ins in your host.
Administrator permission may be required for that folder.
