# BLP Bitcrusher

BLP Bitcrusher is a 64-bit Windows VST3 distortion effect for mono and stereo tracks. It reduces the effective sample rate with sample-and-hold, quantizes the held samples, and blends the result with the dry input. It reports zero samples of latency.

## Install the compiled release

Extract the Windows x64 ZIP. Copy the entire **BLP Bitcrusher.vst3** folder to **C:\Program Files\Common Files\VST3** and rescan VST3 plug-ins in your DAW. Windows may ask for administrator approval to copy into that folder. Look for **BLP Bitcrusher** by **BLP** in the effects list.

## Controls

| Control | Default | Range | Effect |
| --- | --- | --- | --- |
| Intensity | 49% | 0–100% | Increases the time between captured samples. At 0%, the effect captures every sample. |
| Bits | 16 | 1–16 | Sets the wet signal's quantization resolution. One bit produces two levels for nonzero input. |
| Mix | 70% | 0–100% | Blends the dry and crushed signals. |
| Bypass | Off | Off or on | Ramps to the unchanged dry signal over 10 ms while retaining the other settings. |

Drag up or down on the Intensity line or on either lower slider. You can also scroll over a slider to adjust it. The synthetic purple preview updates when Intensity or Bits changes, including during automation. It exaggerates the vertical spacing between quantization levels so the change is visible in the compact display.

## Build on Windows

Use Visual Studio 2022 with the **Desktop development with C++** workload and Windows SDK, plus CMake 3.22 or newer. Run these commands from the source directory.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BLPBitcrusher_VST3 --parallel 1
```

CMake fetches JUCE 8.0.12 on first configure. For an offline build, pass `-DBLP_JUCE_SOURCE=C:/path/to/JUCE` pointing at a local copy of that release.

The build creates **build/BLPBitcrusher_artefacts/Release/VST3/BLP Bitcrusher.vst3**.

## Plug-in identity

- Vendor and product codes are `BlpA` and `Bc7R`.
- The bundle identifier is `com.blp.bitcrusher`.
- The VST3 category is `Fx|Distortion`.
- Parameter IDs are `intensity`, `bits`, `mix`, and `bypass`.

JUCE licensing terms are available in the [JUCE 8.0.12 license](https://github.com/juce-framework/JUCE/blob/8.0.12/LICENSE.md).



