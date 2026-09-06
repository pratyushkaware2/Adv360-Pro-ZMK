#!/usr/bin/env python3
"""Guard the approved layout against Legacy's omission of reserved layers."""
import re
import sys
from pathlib import Path

source = Path(sys.argv[1] if len(sys.argv) > 1 else "config/adv360.keymap").read_text()
source = re.sub(r"//[^\n]*|/\*.*?\*/", "", source, flags=re.S)
keymap = source.split('compatible = "zmk,keymap";', 1)[1]
nodes = re.findall(r"(\w+)\s*\{([^{}]*)\};", keymap, re.S)
# Legacy uses DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP, not all child nodes.
layers = [(name, body) for name, body in nodes
          if not re.search(r'status\s*=\s*"(?!okay"|ok")[^"]+"', body)]
expected = ["windows", "macos", "linux", "unused_3", "unused_4",
            "numbers", "function", "pointer", "system", "reserved"]
assert [name for name, _ in layers] == expected, "Legacy layer indices shifted: keep unused slots enabled"
parsed = []
for name, body in layers:
    bindings = re.search(r"bindings\s*=\s*<(.*?)>;", body, re.S).group(1)
    keys = [" ".join(b.split()) for b in re.findall(r"&[^&]+", bindings)]
    assert len(keys) == 76, (name, len(keys))
    parsed.append(keys)
for i, keys in enumerate(parsed[:3]):
    assert keys[7] == "&mo 8"
    assert keys[28] == "&kp BACKSPACE"
    assert keys[65] == ("&kp LGUI" if i == 1 else "&kp LCTRL")
    assert keys[66] == "&kp LCTRL"
    assert keys[69] == ("&win_hyper" if i == 0 else "&kp LS(LC(LA(LGUI)))")
assert parsed[8][1:5] == ["&host_mac", "&host_linux", "&host_windows", "&host_server"]
assert parsed[8][20:22] == ["&bootloader", "&bootloader"]
for index in (3, 4, 9):
    assert set(parsed[index]) == {"&trans"}
for layer in re.findall(r"&(?:mo|to)\s+(\d+)", source):
    assert int(layer) < len(layers)
for host, bt, layer in (("mac", 0, 1), ("linux", 1, 2), ("windows", 2, 0), ("server", 3, 2)):
    body = re.search(r"host_" + host + r":\s*host_" + host + r"\s*\{(.*?)\};", source, re.S).group(1)
    assert re.search(r"bindings\s*=\s*<&bt BT_SEL " + str(bt) + r">,\s*<&to " + str(layer) + r">;", body)
print("Legacy indices, device macros, bootloader keys and thumb bindings verified.")

hyper = re.search(r"win_hyper:\s*win_hyper\s*\{(.*?)\};", source, re.S).group(1)
sequence = re.search(r"bindings\s*=\s*(.*?);", hyper, re.S).group(1)
sequence = " ".join(re.sub(r"[<>,]", " ", sequence).split())
assert sequence == ("&macro_press &kp LCTRL &kp LSHFT &kp LALT &kp F24 "
                    "&macro_pause_for_release "
                    "&macro_release &kp F24 &kp LALT &kp LSHFT &kp LCTRL"), "Windows Hyper must hold explicit modifiers and release each exactly once"
assert re.search(r"wait-ms\s*=\s*<0>", hyper)
print("Windows Hyper explicitly holds modifiers across command keys and releases them on thumb-up.")
