#!/usr/bin/env python3
"""Exercise the actual driver callbacks with host-side layer/HID stubs."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = root / "local-modules/mouse-mode"
source = (module / "src/behavior_mouse_mode.c").read_text()
callbacks = source[source.index("static int pressed("):source.index("static int cleanup_listener(")]
harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#define MOUSE_LAYER 7
#define ZMK_BEHAVIOR_OPAQUE 0
struct zmk_behavior_binding { unsigned int param1; };
struct zmk_behavior_binding_event { int unused; };
static struct mouse_state state;
static bool active, buttons[3], movement[4];
static void emit_button(unsigned int n, bool down) {
    assert(buttons[n] != down);
    buttons[n] = down;
}
static void emit_move(unsigned int n, bool down) {
    assert(movement[n] != down);
    movement[n] = down;
}
static bool zmk_keymap_layer_active(int layer) { assert(layer == 7); return active; }
static int zmk_keymap_layer_activate(int layer) { assert(layer == 7); active = true; return 0; }
static int exit_mouse(void) { mouse_clear(&state, emit_button, emit_move); active = false; return 0; }
'''
tests = r'''
static void key(unsigned int command, bool down) {
    struct zmk_behavior_binding binding = {.param1 = command};
    struct zmk_behavior_binding_event event = {0};
    if (down) pressed(&binding, event); else released(&binding, event);
}
static void tap(unsigned int command) { key(command, true); key(command, false); }
int main(void) {
    tap(MOUSE_TOGGLE);
    assert(active); // toggle key-up must NOT exit
    tap(MOUSE_DRAG);
    key(MOUSE_RIGHT, true);
    assert(buttons[0] && movement[3]);
    tap(MOUSE_TOGGLE);
    assert(!active && !buttons[0] && !movement[3]); // exit drops drag and motion
    key(MOUSE_RIGHT, false); // late release must not emit twice
    tap(MOUSE_DRAG);
    assert(!buttons[0]); // inactive commands cannot start a drag
    tap(MOUSE_TOGGLE);
    assert(active && !state.dragging);
    key(MOUSE_LEFT_CLICK, true);
    tap(MOUSE_EXIT);
    assert(!active && !buttons[0]);
    key(MOUSE_LEFT_CLICK, false);
    tap(MOUSE_TOGGLE);
    tap(MOUSE_DRAG);
    tap(MOUSE_DRAG);
    assert(active && !buttons[0]); // Y changes drag, not Mouse mode
    tap(MOUSE_TOGGLE);
    assert(!active);
    puts("Actual Mouse callbacks: toggle key-up, drag, toggle-off, Escape and late releases passed.");
}
'''
includes = "\n".join("#include " + json.dumps(str(module / path)) for path in
                     ["include/dt-bindings/zmk/mouse-mode.h", "src/mouse_state.h"])
with tempfile.TemporaryDirectory(prefix="adv360-mouse-test-") as temp:
    c_file = Path(temp) / "callbacks.c"
    executable = Path(temp) / "callbacks"
    c_file.write_text(includes + harness + callbacks + tests)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra",
                    "-Werror", "-Wno-unused-parameter", str(c_file), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
