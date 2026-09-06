// Exercise the exact state machine used in the firmware, without Zephyr.
#include <assert.h>
#include <stdio.h>
#include "../local-modules/mouse-mode/src/mouse_state.h"
static int buttons[3], moves[4], reports;
static void button(unsigned int index, bool down) {
    assert(buttons[index] != down);
    buttons[index] = down;
    reports++;
}
static void move(unsigned int index, bool down) {
    assert(moves[index] != down);
    moves[index] = down;
}
int main(void) {
    struct mouse_state state = {0};
    mouse_drag(&state, button);
    assert(buttons[0]);
    mouse_button(&state, 0, true, button);
    mouse_button(&state, 0, false, button);
    assert(buttons[0] && reports == 1); // click release cannot undo drag
    mouse_drag(&state, button);
    assert(!buttons[0]);
    mouse_button(&state, 0, true, button);
    mouse_drag(&state, button);
    mouse_drag(&state, button);
    assert(buttons[0]); // physical click still held
    mouse_button(&state, 0, false, button);
    assert(!buttons[0]);
    for (unsigned int i = 0; i < 3; i++) mouse_button(&state, i, true, button);
    for (unsigned int i = 0; i < 4; i++) mouse_move(&state, i, true, move);
    mouse_drag(&state, button);
    mouse_clear(&state, button, move); // mode release, Escape or host switch
    int after_clear = reports;
    mouse_clear(&state, button, move);
    for (unsigned int i = 0; i < 3; i++) {
        mouse_button(&state, i, false, button);
        assert(!buttons[i]);
    }
    for (unsigned int i = 0; i < 4; i++) {
        mouse_move(&state, i, false, move);
        assert(!moves[i]);
    }
    assert(reports == after_clear && !state.dragging);
    mouse_drag(&state, button); // re-entry starts a fresh drag
    assert(buttons[0]);
    mouse_clear(&state, button, move);
    puts("Mouse drag, overlapping clicks, movement cleanup and late releases passed.");
}
