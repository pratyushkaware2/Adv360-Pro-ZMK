# Advantage360 toggle Mouse mode

Tap left circled ② (the bottom inner-left key beside G) to toggle Mouse mode.
H/J/K/L control movement,
U/I for left/right click and O for middle click. Tap Y to latch left click;
tap Y again to release it. Tapping ② again or pressing Escape stops
movement, releases all owned buttons and restores typing. Numbers moved
to the bottom inner-right key (beside H). Big thumb keys are unchanged.

This runs in the keyboard and emits ordinary HID mouse reports over Bluetooth
or USB, so no OS mouse-key software is required. Hardware testing on each host
is still required. Disconnecting a host cannot guarantee delivery of a release.

The driver owns one HID reference per button, combines U and Y with OR logic,
and delegates movement acceleration to the vendor mmv behavior. Toggle-off
releases tracked directions and buttons exactly once, including when physical
keys are still down. Host-selection macros clear state and wait 50 ms before
BT_SEL to allow the old host's release report to drain. This delay is best effort.

Build with `-DZMK_EXTRA_MODULES="${PWD}/local-modules/mouse-mode"` in addition
to the existing config and EXTRA_CONF_FILE arguments. Keep the module outside
`modules/`, which the firmware workflow restores from the west dependency cache.
The custom binding may appear unknown in generic keymap editors; retain its
numeric command values and the module when editing or building.

Run `python3 bin/check_keymap.py` and compile/run `bin/test_mouse_state.c` and run `python3 bin/test_mouse_mode.py`
with a C99 host compiler. CI also builds both halves in Legacy and Clique.

Smoke test on a blank desktop: movement in all four directions; U and I clicks;
Y then movement then Y for drag; tap ② off during movement/drag; Escape;
re-enter and confirm no stale click; Mod+1/2/3/4 and ordinary typing/thumb keys.
Do not infer deployment from a successful build.

## Scrolling and speed controls

While Mouse is active, N/M/comma/period scroll left/down/up/right. These are
ordinary horizontal and vertical HID wheel events (10 steps/second); OS and
application scroll preferences still affect the result. No host remapper or
high-resolution scrolling option is required.

Hold D for constant precision movement (150 HID counts/second), or F for
constant fast movement (1200 counts/second). Normal movement remains the
vendor mmv behavior (600 counts/second maximum with a 300 ms ramp). These are
relative HID units, not guaranteed screen pixels. Precision wins while both
D and F are held; releasing it resumes Fast if F is still held. Releasing
both restores Normal. Controls affect cursor movement, not wheel speed or
host modifiers. Both speed changes and diagonals work during a Y-latched drag.

The wrapper releases the previous movement behavior with its original value
before activating the replacement. It owns scrolling releases too. Toggle-off,
Escape, layer deactivation and endpoint changes clear motion, wheel actions,
buttons and speed controls. Late physical key releases cannot restart motion.
The two constant-speed behaviors need their input listeners from the keymap;
keep them alongside the module. Normal movement still delegates to vendor mmv.

Test after flashing on each host: HJKL normal movement; hold D and F before
and during motion; N/M/comma/period wheel direction; Y drag with speed changes;
exit while moving/scrolling/dragging; select another host. Re-entry must start
at normal speed with no button latched. Host-side tests cannot validate radio
delivery, pointer gain or physical feel.
