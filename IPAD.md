# iPad keyboard layout

Mod+5 selects Bluetooth slot 5 and a dedicated iPad base (layer 3). The large right Hyper thumb sends Caps Lock; set **Settings > General > Keyboard > Hardware Keyboard > Modifier Keys > Caps Lock Key > Globe** on the iPad. This is the logical Caps Lock signal, not the physical Caps-position key, which still sends Backspace. Until configured, the thumb toggles Caps Lock.

Command + Control stay on the large left thumbs. Space and all typing, Fn/media, mouse and other host bindings stay unchanged. Mod+1 restores the separate Mac layout with Hyper. Reselect Mod+number after keyboard power-on. Do not clear Bluetooth pairings.

## Everyday controls

- Command+Tab: switch between Obsidian and the video/browser.
- Command+Space: find an app.
- Globe+M: menu bar and available shortcuts.
- Globe+F: toggle full screen.
- Control+Globe+Left/Right: tile the current window to that half.
- Control+Shift+Globe+Left/Right: arrange current and previous windows side by side.
- Fn+U: media Play/Pause (test with your video app while Obsidian is focused). Fn is the top inner-left utility key, not Globe.

Apple shortcut source: https://support.apple.com/en-us/102393
Modifier settings: https://support.apple.com/guide/ipad/use-shortcuts-ipaddf61a0c2/ipados

## Why the Caps Lock carrier

The pinned fork defines native GLOBE as consumer usage 0x029D, but both vendor board defconfigs select BASIC consumer reports with maximum usage 0xFF. Switching to FULL would alter the common HID descriptor across all hosts. The iPad-only CAPSLOCK carrier (keyboard usage 0x39) uses the existing keyboard report and keeps shared transport/configuration intact. No native GLOBE keycode or descriptor change is shipped.

## Validation and physical setup

Automated checks guard layer indices, host selectors, all 76 key positions and the one intentional difference from Mac, plus existing mouse cleanup and Windows Hyper behavior. CI builds Legacy/Clique for both halves. Build success does not verify physical Globe mapping, focus, media targeting, or Bluetooth on iPad. After flashing, configure Modifier Keys, then test Globe+M/F and Control+Globe+Arrow, Command/Control, Backspace, Fn+U, mouse toggle and return to Mod+1. Keep previous firmware for rollback.
