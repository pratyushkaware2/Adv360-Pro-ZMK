// SPDX-License-Identifier: MIT
#pragma once
#include <stdbool.h>
#include <stddef.h>

enum mouse_speed { MOUSE_SPEED_NORMAL, MOUSE_SPEED_PRECISE, MOUSE_SPEED_FAST };
struct mouse_state {
    bool pressed[3];
    bool reported[3];
    bool moving[4];
    bool scrolling[4];
    bool dragging;
    bool precise;
    bool fast;
};

typedef void (*mouse_emit)(unsigned int index, bool down);
typedef void (*mouse_motion_emit)(unsigned int direction, enum mouse_speed speed, bool down);

static inline enum mouse_speed mouse_speed(const struct mouse_state *state) {
    // Precision wins if both controls are held.
    return state->precise ? MOUSE_SPEED_PRECISE : state->fast ? MOUSE_SPEED_FAST : MOUSE_SPEED_NORMAL;
}

static inline void mouse_sync(struct mouse_state *state, unsigned int button, mouse_emit emit) {
    bool down = state->pressed[button] || (button == 0 && state->dragging);
    if (down != state->reported[button]) {
        state->reported[button] = down;
        emit(button, down);
    }
}

static inline void mouse_button(struct mouse_state *state, unsigned int button, bool down,
                                mouse_emit emit) {
    state->pressed[button] = down;
    mouse_sync(state, button, emit);
}

static inline void mouse_drag(struct mouse_state *state, mouse_emit emit) {
    state->dragging = !state->dragging;
    mouse_sync(state, 0, emit);
}

static inline void mouse_move(struct mouse_state *state, unsigned int direction, bool down,
                              mouse_motion_emit emit) {
    if (state->moving[direction] != down) {
        state->moving[direction] = down;
        emit(direction, mouse_speed(state), down);
    }
}

static inline void mouse_scroll(struct mouse_state *state, unsigned int direction, bool down,
                                mouse_emit emit) {
    if (state->scrolling[direction] != down) {
        state->scrolling[direction] = down;
        emit(direction, down);
    }
}

static inline void mouse_set_speed(struct mouse_state *state, bool precise, bool down,
                                   mouse_motion_emit emit) {
    enum mouse_speed before = mouse_speed(state);
    if (precise) state->precise = down;
    else state->fast = down;
    enum mouse_speed after = mouse_speed(state);
    if (before == after) return;
    // Release the exact old behavior/value before starting the replacement.
    // This works even when speed changes during a diagonal move or drag.
    for (unsigned int i = 0; i < 4; i++) {
        if (state->moving[i]) emit(i, before, false);
    }
    for (unsigned int i = 0; i < 4; i++) {
        if (state->moving[i]) emit(i, after, true);
    }
}

static inline void mouse_clear(struct mouse_state *state, mouse_emit button,
                               mouse_motion_emit move, mouse_emit scroll) {
    // Stop motion before dropping a drag. Late physical releases are no-ops.
    for (unsigned int i = 0; i < 4; i++) {
        mouse_move(state, i, false, move);
        mouse_scroll(state, i, false, scroll);
    }
    state->precise = state->fast = false;
    state->dragging = false;
    for (unsigned int i = 0; i < 3; i++) mouse_button(state, i, false, button);
}
