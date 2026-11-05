# BLP Vocal Enhancer 1.0.0

A 64-bit Windows VST3 audio effect by BLP. It supports mono and stereo tracks
at 44.1, 48, 88.2, 96, and 192 kHz and reports zero samples of latency.

- **Mix** defaults to 50%. Drag vertically or scroll to adjust, hold Shift for
  fine adjustment, and double-click to reset. At 0% the audio is dry.
- **Mode** defaults to Warm. Warm adds body and smooths the top end. Standard
  adds balanced clarity. Modern adds punch and air with a stereo-linked
  de-esser. General adds a gentle low and high shelf.
- **Bypass** uses the text button. Orange indicates active processing. Gray
  indicates unity-gain dry audio after the short fade.

All three controls support DAW automation and are stored in project and
host-preset state. The processing blends time-aligned dry and wet audio.
Minimum-phase EQ changes phase near the EQ bands as part of its response.

## Build on Windows

1. Install Visual Studio 2022 with **Desktop development with C++** and a
   Windows 10 or 11 SDK. Install Git and CMake 3.22 or later and add both to PATH.
2. Open PowerShell in this source folder. Configure and build:

   ```powershell
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release --parallel
   ```

   The first command downloads JUCE 8.0.12. To use a local JUCE 8.0.12 checkout,
   append `-DBLP_JUCE_DIR="C:/path/to/JUCE"` to the first command. Use a fresh
   build folder when switching JUCE checkouts.

3. Find the full bundle at
   `build/BLPVocalEnhancer_artefacts/Release/VST3/BLP Vocal Enhancer.vst3`.

## Install and scan

Extract the compiled zip if using the prebuilt version. Copy the entire
**BLP Vocal Enhancer.vst3** folder to
`C:\Program Files\Common Files\VST3\`. Windows may ask for administrator
approval. Keep the bundle's internal folders intact.

Rescan VST3 plug-ins in your DAW's plug-in manager, or restart the host if it
scans at launch. Search for **BLP Vocal Enhancer** under audio effects and
insert it on a vocal track. In FL Studio use **Options > Manage plugins**.
In REAPER use **Preferences > Plug-ins > VST**. In Cubase use the **VST
Plug-in Manager**. In Cakewalk, use the application's VST scan or plug-in
manager. Ensure the standard VST3 folder is included if the host allows
custom scan paths.

## Source layout

The processor handles host buses, automation, and state. The editor binds
the three controls to those parameters. The DSP prepares filter coefficients
outside the audio callback and uses separate filter history per channel.
It crossfades continuously running modes so even rapid switches retain
consistent filter history. The wet path has fixed level trims and a gentle
peak knee. Dry and settled bypass paths pass samples through directly.

