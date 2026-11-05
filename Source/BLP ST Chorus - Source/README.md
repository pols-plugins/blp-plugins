# BLP ST Chorus

BLP ST Chorus is a Windows x64 VST3 audio effect for mono and stereo tracks. Drag Speed or Depth up or down to adjust it. Double-click Speed to restore 60.0% or Depth to restore 100%. The Bypass button fades to dry audio over 30 ms.

Speed maps quadratically from 0 to 8 Hz. At 0%, the LFO holds its current phase. Depth sets the delay excursion around 15 ms, up to ±8 ms. At 0% Depth the fixed delay still colors the sound. The left and right sine modulators are 90° apart. The processor sums dry audio and a half-level delayed voice, then normalizes their combined gain. There is no feedback.

## Install the Release build

Extract the Windows x64 ZIP. Copy the entire **BLP ST Chorus.vst3** folder to **C:\Program Files\Common Files\VST3**. Rescan VST3 effects in your DAW and insert **BLP ST Chorus** on an audio track.

## Build from source

Install Visual Studio 2022 with the Desktop development with C++ workload and a Windows SDK. Install CMake 3.22 or newer and Git. From this source directory, run the following in PowerShell.

~~~powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPSTChorus_VST3
~~~

CMake fetches JUCE 8.0.6, which includes the VST3 SDK. For an existing JUCE 8.0.6 checkout, add `-DJUCE_SOURCE_DIR=C:/path/to/JUCE` to the configure command. The build creates the installable bundle at `build/BLPSTChorus_artefacts/Release/VST3/BLP ST Chorus.vst3`.

JUCE is a separate dependency. Review its license terms before redistributing a build.
