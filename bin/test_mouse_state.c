// Exercise the exact state machine used in the firmware, without Zephyr.
#include <assert.h>
#include <stdio.h>
#include "../local-modules/mouse-mode/src/mouse_state.h"
static int buttons[3], moves[3][4], scrolls[4], reports;
static void button(unsigned int index, bool down) {
    assert(buttons[index] != down);
    buttons[index] = down;
    reports++;
}
static void move(unsigned int index, enum mouse_speed speed, bool down) {
    assert(moves[speed][index] != down);
    moves[speed][index] = down;
}
static void scroll(unsigned int index, bool down) {
    assert(scrolls[index] != down);
    scrolls[index] = down;
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
    mouse_clear(&state, button, move, scroll); // mode release, Escape or host switch
    int after_clear = reports;
    mouse_clear(&state, button, move, scroll);
    for (unsigned int i = 0; i < 3; i++) {
        mouse_button(&state, i, false, button);
        assert(!buttons[i]);
    }
    for (unsigned int i = 0; i < 4; i++) {
        mouse_move(&state, i, false, move);
        assert(!moves[0][i] && !moves[1][i] && !moves[2][i]);
    }
    assert(reports == after_clear && !state.dragging);
    mouse_drag(&state, button); // re-entry starts a fresh drag
    assert(buttons[0]);
    mouse_clear(&state, button, move, scroll);
    // Speed can change with two directions and drag already active.
    mouse_drag(&state, button);
    mouse_move(&state, 0, true, move);
    mouse_move(&state, 2, true, move);
    mouse_set_speed(&state, false, true, move);
    assert(!moves[0][0] && moves[2][0] && moves[2][2] && buttons[0]);
    mouse_set_speed(&state, true, true, move);
    assert(!moves[2][0] && moves[1][0] && moves[1][2]);
    mouse_set_speed(&state, false, false, move);
    assert(moves[1][0]); // precision wins over fast
    mouse_set_speed(&state, true, false, move);
    assert(moves[0][0] && moves[0][2] && !moves[1][0]);
    for (unsigned int i = 0; i < 4; i++) mouse_scroll(&state, i, true, scroll);
    mouse_set_speed(&state, true, true, move);
    mouse_clear(&state, button, move, scroll);
    mouse_set_speed(&state, true, false, move);
    for (unsigned int i = 0; i < 4; i++) {
        mouse_move(&state, i, false, move);
        mouse_scroll(&state, i, false, scroll);
        assert(!scrolls[i] && !moves[0][i] && !moves[1][i] && !moves[2][i]);
    }
    assert(mouse_speed(&state) == MOUSE_SPEED_NORMAL && !buttons[0]);
    puts("Mouse state: overlapping clicks, drag, diagonal speed changes, scrolling and cleanup passed.");
}
