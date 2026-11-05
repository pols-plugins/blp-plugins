# BLP BL1176

Windows x64 VST3 dynamics effect by BLP.

## Install

Extract the ZIP. Copy the entire **BLP BL1176.vst3** folder to
\`C:\\Program Files\\Common Files\\VST3\`. Rescan plug-ins in your DAW and insert
**BLP BL1176** as an audio effect. Keep the folder structure intact.

## Controls

- **Input** adds 0 to 20 dB before detection. Default is +10 dB.
- **Output** applies -15 to +15 dB after compression. Default is 0 dB.
- **Attack** runs from 800 to 20 microseconds as you turn it up. Default is 1.2.
- **Release** runs from 1100 to 50 milliseconds as you turn it up. Default is 5.0.
- **Ratio** selects 4:1, 8:1, 12:1, 20:1, or All. All uses a lower threshold, a
  near-limiting ratio, and a faster release.
- **Bypass** crossfades to the dry input in 5 ms.

The detector follows the louder channel in stereo and applies the same gain
reduction to both channels. Processing has zero reported latency. The plug-in
uses floating-point headroom. Set Output to suit the level of the next stage.
Factory programs are exposed through the host's VST3 program controls.

## Build

Install Visual Studio 2022 with Desktop development with C++, a Windows SDK,
CMake 3.22 or newer, and Git. From the directory containing CMakeLists.txt run:

\`\`\`powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLP1176_VST3
\`\`\`

CMake fetches JUCE 8.0.8. To use a local checkout, set
\`-DFETCHCONTENT_SOURCE_DIR_JUCE=C:/path/to/JUCE\` on the configure command.
The bundle appears under
\`build/BLP1176_artefacts/Release/VST3/BLP BL1176.vst3\`.

[JUCE 8.0.8](https://github.com/juce-framework/JUCE/tree/8.0.8)
