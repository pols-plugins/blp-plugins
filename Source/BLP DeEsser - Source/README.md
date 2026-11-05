# BLP DeEsser — BLP
Windows x64 VST3. Frequency-selective vocal de-esser.

GUI: vertical Threshold slider, horizontal Frequency slider, editable numeric values and Bypass. Dark reference-inspired background with static decorative dots and a yellow frequency marker. Dots are not a live analyzer.
Amount, Mix, Attack and Release knobs are removed from the GUI. Their existing host parameters remain for automation/state support, with defaults of 60%, 100%, 2 ms and 80 ms respectively.

Install: copy the entire BLP DeEsser.vst3 folder into C:\Program Files\Common Files\VST3, then rescan your DAW. This retains the BLP Graphic EQ plugin identifier for project compatibility. Replace the previous BLP Graphic EQ bundle rather than installing both.

Build: install Visual Studio 2022 C++ tools and CMake 3.22+. Clone JUCE 8.0.6 into the JUCE subfolder. Run:
cmake -S . -B build -A x64
cmake --build build --config Release -- /m:1 /nr:false
For Visual Studio C1902, set $env:_CL_='/Z7' before building in PowerShell.

Original DSP tests passed in the prior version; DSP is unchanged. Live DAW validation is not completed. JUCE is separately licensed under AGPLv3 or commercial terms; see https://juce.com/legal/juce-8-licence/.


Updated controls: Frequency 1000–16000 Hz in 1 Hz steps, no decimals; Threshold -40.0–0.0 dB in 0.1 dB steps; Bypass at top right. Frequency marker follows the expanded range. Existing normalized host automation may map differently because the parameter ranges changed.
Rebuilt successfully and reran the signal/state regression test successfully. Live DAW validation remains incomplete.

New-instance defaults: Frequency 9000 Hz; Threshold -19.0 dB. Existing saved projects retain their stored settings.

The separate BLP author label has been removed from the interface.

Threshold value now follows the white line at its left edge, displayed with one decimal place. Adjust threshold by dragging the slider.
