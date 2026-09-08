#!/usr/bin/env python3
"""Compile the actual driver, including dispatch/cleanup, against host HID stubs."""
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = root / "local-modules/mouse-mode"
source = (module / "src/behavior_mouse_mode.c").read_text()
source = re.sub(r'^#include .*$', '', source, flags=re.M)
source = source[:source.index('ZMK_LISTENER(')]
harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#define DT_INST_PROP(i, prop) 7
#define DT_INST_PHANDLE(i, prop) prop
#define STR_(x) #x
#define STR(x) STR_(x)
#define DEVICE_DT_NAME(x) STR(x)
#define ZMK_BEHAVIOR_OPAQUE 0
#define ZMK_EV_EVENT_BUBBLE 0
#define MOVE_X(x) (((uint32_t)(x) & 0xffff) << 16)
#define MOVE_Y(y) ((uint32_t)(y) & 0xffff)
#define MOVE_LEFT MOVE_X(-600)
#define MOVE_DOWN MOVE_Y(600)
#define MOVE_UP MOVE_Y(-600)
#define MOVE_RIGHT MOVE_X(600)
#define SCRL_LEFT MOVE_X(-10)
#define SCRL_DOWN MOVE_Y(-10)
#define SCRL_UP MOVE_Y(10)
#define SCRL_RIGHT MOVE_X(10)
struct zmk_behavior_binding { const char *behavior_dev; unsigned int param1; };
struct zmk_behavior_binding_event { int layer; int64_t timestamp; };
struct zmk_layer_state_changed { int layer; bool state; };
struct zmk_endpoint_changed { int unused; };
typedef struct { int kind; struct zmk_layer_state_changed layer; struct zmk_endpoint_changed endpoint; } zmk_event_t;
static const struct zmk_layer_state_changed *as_zmk_layer_state_changed(const zmk_event_t *e) {
    return e->kind == 1 ? &e->layer : NULL;
}
static const struct zmk_endpoint_changed *as_zmk_endpoint_changed(const zmk_event_t *e) {
    return e->kind == 2 ? &e->endpoint : NULL;
}
static bool active;
static int buttons[3], velocity[4][2], references[4][4], reports;
static int64_t k_uptime_get(void) { return 123; }
static void zmk_hid_mouse_button_press(unsigned int n) { assert(buttons[n] == 0); buttons[n]++; }
static void zmk_hid_mouse_button_release(unsigned int n) { assert(buttons[n] == 1); buttons[n]--; }
static void zmk_endpoints_send_mouse_report(void) { reports++; }
static bool zmk_keymap_layer_active(int layer) { assert(layer == 7); return active; }
static int zmk_keymap_layer_activate(int layer) { assert(layer == 7); active = true; return 0; }
static int zmk_keymap_layer_deactivate(int layer) { assert(layer == 7); active = false; return 0; }
static int zmk_behavior_invoke_binding(const struct zmk_behavior_binding *b,
                                     struct zmk_behavior_binding_event e, bool down) {
    assert(e.layer == 7);
    const char *names[] = {"movement_behavior", "precise_behavior", "fast_behavior", "scroll_behavior"};
    int behavior = -1;
    for (int i = 0; i < 4; i++) if (!strcmp(b->behavior_dev, names[i])) behavior = i;
    assert(behavior >= 0);
    int x = (int16_t)(b->param1 >> 16), y = (int16_t)b->param1;
    assert((x == 0) != (y == 0));
    int direction = x < 0 ? 0 : x > 0 ? 3 : y > 0 ? 1 : 2;
    assert(references[behavior][direction] == (down ? 0 : 1));
    references[behavior][direction] += down ? 1 : -1;
    velocity[behavior][0] += down ? x : -x;
    velocity[behavior][1] += down ? y : -y;
    return 0;
}
'''
tests = r'''
static void key(unsigned int command, bool down) {
    struct zmk_behavior_binding binding = {.param1 = command};
    struct zmk_behavior_binding_event event = {0};
    if (down) pressed(&binding, event); else released(&binding, event);
}
static void tap(unsigned int command) { key(command, true); key(command, false); }
static void assert_clear(void) {
    assert(!active && !state.dragging && !state.precise && !state.fast);
    for (int i = 0; i < 3; i++) assert(!buttons[i]);
    for (int i = 0; i < 4; i++) {
        assert(!velocity[i][0] && !velocity[i][1]);
        for (int j = 0; j < 4; j++) assert(!references[i][j]);
    }
}
static void release_all(void) {
    for (unsigned int i = MOUSE_LEFT_CLICK; i <= MOUSE_FAST; i++) key(i, false);
}
int main(void) {
    tap(MOUSE_TOGGLE);
    assert(active); // toggle key-up must NOT exit
    tap(MOUSE_DRAG);
    tap(MOUSE_LEFT_CLICK);
    assert(buttons[0]); // a physical click cannot undo latched drag
    key(MOUSE_RIGHT, true);
    key(MOUSE_UP, true);
    assert(velocity[0][0] == 600 && velocity[0][1] == -600);
    key(MOUSE_FAST, true);
    assert(!velocity[0][0] && velocity[2][0] == 1200 && velocity[2][1] == -1200);
    key(MOUSE_PRECISE, true);
    assert(!velocity[2][0] && velocity[1][0] == 150 && velocity[1][1] == -150);
    key(MOUSE_PRECISE, false);
    assert(!velocity[1][0] && velocity[2][0] == 1200);
    key(MOUSE_FAST, false);
    assert(!velocity[2][0] && velocity[0][0] == 600 && buttons[0]);
    key(MOUSE_SCROLL_DOWN, true);
    assert(velocity[3][1] == -10);
    tap(MOUSE_TOGGLE);
    assert_clear();
    release_all(); // physical key-up after cleanup cannot subtract velocity twice
    for (unsigned int i = MOUSE_DRAG; i <= MOUSE_FAST; i++) tap(i);
    assert_clear(); // inactive commands cannot start any action

    // Check every direction and its HID sign separately, including horizontal wheel.
    const int expected[4][2] = {{-10,0}, {0,-10}, {0,10}, {10,0}};
    tap(MOUSE_TOGGLE);
    for (int i = 0; i < 4; i++) {
        key(MOUSE_SCROLL_LEFT + i, true);
        assert(velocity[3][0] == expected[i][0] && velocity[3][1] == expected[i][1]);
        key(MOUSE_SCROLL_LEFT + i, false);
        assert(!velocity[3][0] && !velocity[3][1]);
    }
    tap(MOUSE_EXIT);
    assert_clear();

    // Exercise every exit path with movement, scrolling, both speed keys and drag.
    for (int exit = 0; exit < 4; exit++) {
        tap(MOUSE_TOGGLE);
        key(MOUSE_PRECISE, true);
        key(MOUSE_FAST, true);
        tap(MOUSE_DRAG);
        key(MOUSE_LEFT_CLICK, true);
        key(MOUSE_RIGHT_CLICK, true);
        for (int i = MOUSE_LEFT; i <= MOUSE_SCROLL_RIGHT; i++) key(i, true);
        if (exit == 0) tap(MOUSE_TOGGLE);
        if (exit == 1) tap(MOUSE_EXIT); // also used before BT_SEL
        if (exit == 2) { zmk_event_t e = {.kind = 2}; cleanup_listener(&e); }
        if (exit == 3) {
            active = false;
            zmk_event_t e = {.kind = 1, .layer = {.layer = 7, .state = false}};
            cleanup_listener(&e);
        }
        assert_clear();
        release_all();
        assert_clear();
        tap(MOUSE_TOGGLE);
        key(MOUSE_RIGHT, true);
        assert(velocity[0][0] == 600); // fresh entry always starts at normal speed
        tap(MOUSE_EXIT);
        key(MOUSE_RIGHT, false);
        assert_clear();
    }
    puts("Actual driver: speed routing, wheel signs, overlapping drag, all exit paths and late releases passed.");
}
'''
includes = "\n".join("#include " + json.dumps(str(module / path)) for path in
                     ["include/dt-bindings/zmk/mouse-mode.h", "src/mouse_state.h"])
with tempfile.TemporaryDirectory(prefix="adv360-mouse-test-") as temp:
    c_file = Path(temp) / "driver.c"
    executable = Path(temp) / "driver"
    c_file.write_text(includes + harness + source + tests)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra",
                    "-Werror", "-Wno-unused-parameter", str(c_file), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
