// SPDX-License-Identifier: MIT
#pragma once
#include <stdbool.h>
#include <stddef.h>

struct mouse_state {
    bool pressed[3];
    bool reported[3];
    bool moving[4];
    bool dragging;
};

typedef void (*mouse_emit)(unsigned int index, bool down);

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
                              mouse_emit emit) {
    if (state->moving[direction] != down) {
        state->moving[direction] = down;
        emit(direction, down);
    }
}

static inline void mouse_clear(struct mouse_state *state, mouse_emit button, mouse_emit move) {
    // Stop motion before dropping a drag. Late physical releases are no-ops.
    for (unsigned int i = 0; i < 4; i++) {
        mouse_move(state, i, false, move);
    }
    state->dragging = false;
    for (unsigned int i = 0; i < 3; i++) {
        mouse_button(state, i, false, button);
    }
}
