# BLP EQ3-M

Extract the Windows x64 ZIP and copy the entire BLP EQ3-M.vst3 folder into C:\Program Files\Common Files\VST3. Replace the previous bundle and rescan your DAW's plug-ins.

Mid Sweep ranges from 100 to 8000 in whole Hz and defaults to 8000. Low, Mid, and High display their gain on hover.

To build from source, install Visual Studio 2022 with Desktop development with C++, Git and CMake 3.22 or newer. Run:

    cmake -S . -B build -A x64
    cmake --build build --config Release --target EQ3M_VST3

To use a local JUCE 9.0.5 source tree, pass -DJUCE_DIR=C:/path/to/JUCE when configuring. The bundle is under build/EQ3M_artefacts/Release/VST3.

Project source is provided under AGPL-3.0-or-later. See JUCE-LICENSE.md for JUCE terms.
