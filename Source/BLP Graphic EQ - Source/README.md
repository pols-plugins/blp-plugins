# BLP Graphic EQ 2.0.0

Windows x64 VST3 effect by BLP. Seven fixed-frequency bell filters, Output and Bypass. C++17 / JUCE 8.0.12 / CMake. This is an independent plugin, not affiliated with BandLab.

## Install

Copy the **entire BLP Graphic EQ.vst3 folder** into `C:\Program Files\Common Files\VST3` (Windows may request administrator permission), then rescan VST3 effects in your DAW. Alternatively, put the bundle in a custom VST3 directory supported by your host. Do not extract or move only the inner binary.

Choose **BLP Graphic EQ**, manufacturer **BLP**, category **EQ**. Windows 10/11 64-bit target; no separate JUCE installation is needed to use the binary. FL Studio, Sonar, Next and REAPER compatibility is intended through the VST3 standard; consult TEST-RESULTS.md for actual test coverage.

## Controls

- 100 Hz, 200 Hz, 400 Hz, 800 Hz, 1.6 kHz, 3.2 kHz and 6.4 kHz: -15 to +15 dB, initially 0 dB.
- Level (Output): -15 to +15 dB after the filters, initially 0 dB.
- Bypass: smoothly crossfades to the original input, including bypassing Output.
- Drag the seven vertical sliders, or drag the Level knob. Double-click a control to reset to 0 dB. Hover over a band slider to see its gain; the popup also stays visible while dragging. The output knob keeps its editable numeric value. Use your DAW's automation controls for every band, Output and Bypass.
- Save and load presets using your DAW's plugin preset menu (.vstpreset where supported, or the host's native preset format). Settings are also stored with the DAW project. No separate preset menu is added to the plugin.

Boosting several bands can exceed digital full scale; use Output to maintain headroom. There is no limiter or clipping stage.

## Build

Install Visual Studio 2022 with Desktop development with C++ and a Windows SDK, plus CMake 3.22 or later. From this folder:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 1
ctest --test-dir build -C Release --output-on-failure
& .\build\HostTests_artefacts\Release\HostTests.exe "$PWD\build\GraphicEQ_artefacts\Release\VST3\BLP Graphic EQ.vst3"
```

CMake uses the included ThirdParty/JUCE source if present, otherwise fetches pinned JUCE 8.0.12. You can also pass `-DJUCE_DIR=C:/path/to/JUCE` to use an existing source checkout. The VST3 SDK is included in JUCE. The build uses the static Microsoft runtime.

Output: `build/GraphicEQ_artefacts/Release/VST3/BLP Graphic EQ.vst3`.

## Version 2 changes

Seven vertical sliders and a Level knob in a plain warm-gray 480 x 300 interface, inspired by the supplied reference. Band gain values appear only on hover or while dragging. The output gain value remains visible and editable. Bypass is at the top right. Frequency labels show numbers only, and the separate BLP and Level labels have been removed. All gain controls span -15 to +15 dB. New plugin identifiers and frequency-specific parameter IDs keep this version separate from the earlier Graphic EQ; existing sessions using the old plugin are not silently retuned. Keep the old plugin installed if older projects need it.

## Implementation

Seven RBJ peaking biquads at Q=1.1, including bell filters at both ends. Double-precision filter coefficients and state; float audio I/O. Per-channel state prevents crosstalk. Gains settle using a 25 ms exponential smoother at a 32-sample control rate; coefficients interpolate each sample. Output and bypass use 10 ms exponential smoothing. Filters continue running while bypassed to preserve state when re-enabled. The processing path uses fixed-size storage, performs no allocation, reports zero latency and supports matched mono/stereo buses. Preparation resets state and recalculates coefficients for the sample rate. Frequency clamps protect rates below the intended 44.1-96 kHz range. JUCE disables denormals around processing.

Parameter IDs and version hints are stable. Manufacturer code `Blpa`, plugin code `B7e2`, bundle ID `audio.blp.graphiceq.b7e2`. Do not change these identifiers in an update intended to reopen existing projects.

## Source and licensing

The original Graphic EQ sources are provided under AGPL-3.0-or-later; see LICENSE.txt. JUCE is used under its AGPLv3 option. Third-party notices and sources are included in ThirdParty/JUCE. JUCE's licensing information is at https://github.com/juce-framework/JUCE/blob/8.0.12/LICENSE.md.
