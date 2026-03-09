# Venom OSD Development Plan

- [/] Plan implementation for Audio and Brightness OSD
  - [/] Research PulseAudio and PipeWire C APIs for volume/mute events and control.
  - [/] Research X11 (XRandR or similar) C APIs or standard ways to listen to backlight/brightness changes.
  - [/] Write `implementation_plan.md`
- [/] Implement Audio OSD
  - [/] Initialize PA/PW context and listen to sink events (volume and mute).
  - [/] Update UI to draw volume bar and mute icon/text.
- [/] Implement Brightness OSD
  - [/] Monitor brightness changes (via inotify on `/sys/class/backlight` or X11 events/XRandR).
  - [/] Update UI to draw brightness bar and icon.
- [/] Integration and Final Polish
  - [/] Refactor drawing code to handle different OSD types (Keyboard, Volume, Brightness).
  - [/] Update `Makefile` and `install.sh` for new dependencies.
