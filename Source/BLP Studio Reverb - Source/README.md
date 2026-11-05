# BLP Studio Reverb

Algorithmic reverb recreating a studio space.

BLP Studio Reverb is a VST3 audio effect with mono and stereo layouts. It combines six early-reflection taps with an eight-delay feedback network. Size changes reflection spacing and decay. Tone adjusts damping inside the network and filters the wet output. Mix is a linear crossfade with exact dry and wet endpoints after its 20 ms ramp.

## Install the Windows x64 build

Extract the download and copy the entire BLP Studio Reverb.vst3 folder into your host's VST3 search folder. Rescan plugins and look for BLP Studio Reverb under BLP.

## Build from source

Use CMake 3.22 or newer, Git, and Visual Studio 2022 with Desktop development with C++. Run these commands from this project folder in PowerShell.

    cmake -S . -B build -A x64
    cmake --build build --config Release --target StudioReverb_VST3

CMake fetches JUCE 8.0.6. For an offline build, add -DJUCE_SOURCE_DIR= followed by the path to an existing JUCE 8.0.6 checkout to the first command. If the MSVC program database manager reports C1902, set $env:_CL_='/Z7' in that PowerShell session before configuring a fresh build directory.

Find the resulting bundle in build/StudioReverb_artefacts/Release/VST3.

## Controls

| Control | Default | Range |
|---|---:|---:|
| Mix | 50% | 0–100% |
| Size | 25% | 0–100% |
| Tone | 90% | 0–100% |

Double-click a slider to restore its default. All three controls support host automation and session recall. The Mix halo follows the current parameter value.

Move Mix upward for more reverb. The top-right Bypass button switches to the dry signal with a 20 ms transition and follows the host's bypass parameter. Its state is saved with the session.
