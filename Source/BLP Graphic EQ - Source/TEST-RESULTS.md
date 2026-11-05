# BLP Graphic EQ 2.0.0 validation

Release x64 VST3 build passed. The actual bundle was scanned and loaded by a separate JUCE VST3 test host. Plugin name BLP Graphic EQ and manufacturer BLP were verified.

Passed checks:
- DSP at 44.1, 48, 88.2 and 96 kHz: flat default response, all seven new centre frequencies at -15/+15 dB (0.03 dB tolerance), distant-frequency isolation, Level -15/+15 dB, and unity bypass.
- Stereo isolation and identical processing of identical left/right signals.
- Randomized automation stress at all four sample rates, with no non-finite output or runaway gain.
- Smoothed band, Level and bypass changes on a DC test signal; largest adjacent-sample step 0.000121008 for 0.01 input. This is a specific continuity test, not a guarantee for every possible automation sequence.
- Compiled VST3 mono/stereo processing at all four sample rates; zero reported latency.
- Compiled VST3 automated band and Level endpoint gains, and bypass audio measurements.
- Project-state restoration and a real .vstpreset disk save/load round-trip.
- Editor opens at 480 x 300. Seven vertical sliders and one rotary Level control, all ranging -15 to +15 dB and defaulting/resetting to 0 dB.
- Direct rendering of the same editor implementation visually inspected; see BLP-GraphicEQ-preview.png.

One isolated DSP benchmark processed 10 seconds of stereo 48 kHz audio in 0.0273 seconds, about 0.27% of one core. DAW, device and GUI overhead are excluded.

Interactive DAW testing remains unverified: computer control was stopped by the user during the earlier version's testing, and was not resumed for this revision. FL Studio, Sonar, Next and REAPER compatibility is intended through VST3, not certified here. Manual mouse interaction, listening tests and reopening a DAW project remain unverified. Separate Windows 10/11 machines were not tested.

Run Build-Windows.ps1 to rebuild and repeat automated checks. Logs and complete test source are included.

## GUI cleanup revision

Removed the separate top-right BLP label, moved Bypass to that corner, removed the Level label, shortened the seven frequency labels to numbers, and replaced persistent band gain text with JUCE hover/drag value popups. The output numeric value remains visible. Rebuilt and reran the existing VST3 integration suite successfully; the updated idle editor rendering was visually inspected. Interactive DAW/hover testing remains unverified. Audio settings and plugin/parameter identifiers are unchanged from version 2.0.0.
