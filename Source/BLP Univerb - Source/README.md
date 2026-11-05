# BLP Univerb 1.0.0

BLP Univerb is a 64-bit Windows VST3 reverb. Its eight environments are
algorithmic approximations using early reflections, stereo diffusion and
a damped feedback delay network. Mix starts at 35%, and Type starts at
Women's Institute. Double-click Mix to reset it.

## Install

Extract the release ZIP and copy the entire BLP Univerb.vst3 folder into
your DAW's VST3 scan folder. The standard system folder is
C:\Program Files\Common Files\VST3. Rescan plug-ins in your DAW, then
insert BLP Univerb as an audio effect. Keep the bundle contents together.

## Build the source

Use Visual Studio 2022 with Desktop development with C++, a Windows SDK,
and CMake 3.22 or later. Run these commands from the source folder:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target BLPUniverb_VST3 --parallel
```

CMake downloads JUCE 8.0.12. For an offline build, pass
`-DBLP_JUCE_SOURCE=C:/path/to/JUCE` during configuration. The VST3 bundle is
under `build/BLPUniverb_artefacts/Release/VST3`.
