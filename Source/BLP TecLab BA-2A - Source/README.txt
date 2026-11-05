BLP TecLab BA-2A — BLP

INSTALL: Copy the entire BLP TecLab BA-2A.vst3 folder into C:\Program Files\Common Files\VST3, then rescan plugins in your DAW. Windows x64 only.

Gain is displayed as 0–100%, controlling compression-dependent makeup. Its nominal -12 to +24 dB range is scaled by actual reduction (up to full strength at 24 dB reduction). At the default 60%, it restores 40% of reduction up to +9.6 dB. Squeeze at 0% applies no compression or makeup gain; with Output at its default 0 dB, the signal is unchanged after the 30 ms parameter smoothing settles. Higher Squeeze progressively increases compression. Squeeze defaults to 70%. Mix defaults to 100%, Output to 0 dB, mode to Compress. The editor shows only Gain and Squeeze knobs, with percentage values shown on hover. Bypass is at the top right. Mix and Output remain available as host parameters for existing project compatibility. Double-click resets a knob. Bypass crossfades to untouched input. The final soft safety saturation prevents processed output from exceeding full scale; bypass preserves original audio. No lookahead or latency.

The preset selector has been removed from the editor. Save/load custom presets using your DAW's VST3 preset or project-state facilities. All six parameters support host automation. The algorithm is an independent optical-style approximation, not an emulation of proprietary BandLab code.

BUILD: Install Visual Studio 2022 C++ Desktop tools, CMake 3.22+, and JUCE source. From this source directory:
cmake -S . -B build -A x64 -DJUCE_DIR="C:/path/to/JUCE"
cmake --build build --config Release --parallel
Result: build/BLPTecLab_artefacts/Release/VST3
Run build/BLPTests_artefacts/Release/BLPTests.exe for DSP smoke checks.
JUCE is an external dependency subject to its own licensing; obtain the appropriate JUCE license for distribution.

Testing: See Test-results.txt. Compilation and synthetic signal checks do not establish compatibility with every DAW. In-DAW loading and listening tests remain required.


