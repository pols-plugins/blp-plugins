# BLP Pitch Shifter

A 64-bit Windows VST3 audio effect by BLP. The default sound is Vocal mode, shifted down five semitones, with 100% wet mix and the effect enabled.

## Install

Extract the compiled archive. Copy the entire **BLP Pitch Shifter.vst3** folder into `C:\Program Files\Common Files\VST3\` and rescan VST3 plug-ins in your DAW. Windows may ask for administrator permission when you copy into that folder. Keep the contents of the bundle together.

## Build from source

Install Visual Studio 2022 with Desktop development with C++, a Windows SDK, CMake 3.24 or newer, and Git. From this directory run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPPitchShifter_VST3
```

The bundle is generated at `build/BLPPitchShifter_artefacts/Release/VST3/BLP Pitch Shifter.vst3`. CMake fetches JUCE 8.0.6 and the pinned Signalsmith Stretch revision. Stretch fetches Signalsmith Linear 0.6.6. For an offline build with those dependencies already available locally, set `FETCHCONTENT_SOURCE_DIR_JUCE`, `FETCHCONTENT_SOURCE_DIR_STRETCH`, and `FETCHCONTENT_SOURCE_DIR_SIGNALSMITH-LINEAR` to their source directories at configure time.

## Signal path

Vocal uses a 100 ms analysis window and spectral formant compensation. General uses a 75 ms window. Percussion uses a 40 ms window and a shorter hop for better time resolution. The engines run at a 1:1 input-to-output sample count and use linked stereo analysis. Their outputs and the dry path share one reported latency of about 100 ms. The plug-in keeps that latency fixed as you change modes, mix, or bypass. Bypass returns the original samples with the reported delay.

All four controls are automatable and saved with the DAW project. Shift uses whole semitones. Mix uses linear gains. Mode changes crossfade, and Shift, Mix and Bypass are smoothed.

## Dependencies

- JUCE modules are available under AGPLv3 or the JUCE commercial license. The bundled VST3 SDK has separate terms in its upstream license.
- Signalsmith Stretch and Signalsmith Linear are MIT licensed. See their included notices.

The source archive contains the pinned dependencies. To use them without downloads, add these arguments to the configure command:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  "-DFETCHCONTENT_SOURCE_DIR_JUCE=$PWD/Dependencies/JUCE" `
  "-DFETCHCONTENT_SOURCE_DIR_STRETCH=$PWD/Dependencies/Stretch" `
  "-DFETCHCONTENT_SOURCE_DIR_SIGNALSMITH-LINEAR=$PWD/Dependencies/Linear"
```



