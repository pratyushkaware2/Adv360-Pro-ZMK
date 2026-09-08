// SPDX-License-Identifier: MIT
#define DT_DRV_COMPAT zmk_behavior_mouse_mode
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <dt-bindings/zmk/mouse-mode.h>
#include <dt-bindings/zmk/pointing.h>
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include "mouse_state.h"

#define MOUSE_LAYER DT_INST_PROP(0, layer)
static struct mouse_state state;
static const uint32_t directions[][4] = {
    {MOVE_LEFT, MOVE_DOWN, MOVE_UP, MOVE_RIGHT},
    {MOVE_X(-150), MOVE_Y(150), MOVE_Y(-150), MOVE_X(150)},
    {MOVE_X(-1200), MOVE_Y(1200), MOVE_Y(-1200), MOVE_X(1200)},
};
static const uint32_t scroll_directions[] = {SCRL_LEFT, SCRL_DOWN, SCRL_UP, SCRL_RIGHT};
static const char *const movement_behaviors[] = {
    DEVICE_DT_NAME(DT_INST_PHANDLE(0, movement_behavior)),
    DEVICE_DT_NAME(DT_INST_PHANDLE(0, precise_behavior)),
    DEVICE_DT_NAME(DT_INST_PHANDLE(0, fast_behavior)),
};

static void emit_button(unsigned int button, bool down) {
    // Own one HID reference per logical button. U and Y share that reference,
    // so releasing U never accidentally drops an active Y drag.
    if (down) {
        zmk_hid_mouse_button_press(button);
    } else {
        zmk_hid_mouse_button_release(button);
    }
    zmk_endpoints_send_mouse_report();
}

static void emit_move(unsigned int direction, enum mouse_speed speed, bool down) {
    const struct zmk_behavior_binding binding = {
        .behavior_dev = movement_behaviors[speed],
        .param1 = directions[speed][direction],
    };
    const struct zmk_behavior_binding_event event = {
        .layer = MOUSE_LAYER, .timestamp = k_uptime_get(),
    };
    zmk_behavior_invoke_binding(&binding, event, down);
}

static void emit_scroll(unsigned int direction, bool down) {
    const struct zmk_behavior_binding binding = {
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE(0, scroll_behavior)),
        .param1 = scroll_directions[direction],
    };
    const struct zmk_behavior_binding_event event = {
        .layer = MOUSE_LAYER, .timestamp = k_uptime_get(),
    };
    zmk_behavior_invoke_binding(&binding, event, down);
}

static void clear_mouse(void) {
    mouse_clear(&state, emit_button, emit_move, emit_scroll);
}

static int exit_mouse(void) {
    clear_mouse();
    return zmk_keymap_layer_deactivate(MOUSE_LAYER);
}

static int pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case MOUSE_TOGGLE:
        return zmk_keymap_layer_active(MOUSE_LAYER) ? exit_mouse()
                                                  : zmk_keymap_layer_activate(MOUSE_LAYER);
    case MOUSE_EXIT:
        return exit_mouse();
    default:
        break;
    }
    if (!zmk_keymap_layer_active(MOUSE_LAYER)) {
        return ZMK_BEHAVIOR_OPAQUE;
    }
    if (binding->param1 == MOUSE_DRAG) {
        mouse_drag(&state, emit_button);
    } else if (binding->param1 >= MOUSE_LEFT_CLICK && binding->param1 <= MOUSE_MIDDLE_CLICK) {
        mouse_button(&state, binding->param1 - MOUSE_LEFT_CLICK, true, emit_button);
    } else if (binding->param1 >= MOUSE_LEFT && binding->param1 <= MOUSE_RIGHT) {
        mouse_move(&state, binding->param1 - MOUSE_LEFT, true, emit_move);
    } else if (binding->param1 >= MOUSE_SCROLL_LEFT && binding->param1 <= MOUSE_SCROLL_RIGHT) {
        mouse_scroll(&state, binding->param1 - MOUSE_SCROLL_LEFT, true, emit_scroll);
    } else if (binding->param1 == MOUSE_PRECISE || binding->param1 == MOUSE_FAST) {
        mouse_set_speed(&state, binding->param1 == MOUSE_PRECISE, true, emit_move);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    if (binding->param1 >= MOUSE_LEFT_CLICK && binding->param1 <= MOUSE_MIDDLE_CLICK) {
        mouse_button(&state, binding->param1 - MOUSE_LEFT_CLICK, false, emit_button);
    } else if (binding->param1 >= MOUSE_LEFT && binding->param1 <= MOUSE_RIGHT) {
        mouse_move(&state, binding->param1 - MOUSE_LEFT, false, emit_move);
    } else if (binding->param1 >= MOUSE_SCROLL_LEFT && binding->param1 <= MOUSE_SCROLL_RIGHT) {
        mouse_scroll(&state, binding->param1 - MOUSE_SCROLL_LEFT, false, emit_scroll);
    } else if (binding->param1 == MOUSE_PRECISE || binding->param1 == MOUSE_FAST) {
        mouse_set_speed(&state, binding->param1 == MOUSE_PRECISE, false, emit_move);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int cleanup_listener(const zmk_event_t *event) {
    const struct zmk_layer_state_changed *layer = as_zmk_layer_state_changed(event);
    if (layer && layer->layer == MOUSE_LAYER && !layer->state) {
        clear_mouse();
    }
    if (as_zmk_endpoint_changed(event)) {
        exit_mouse();
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(mouse_mode_cleanup, cleanup_listener);
ZMK_SUBSCRIPTION(mouse_mode_cleanup, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(mouse_mode_cleanup, zmk_endpoint_changed);

static const struct behavior_driver_api api = {
    .binding_pressed = pressed,
    .binding_released = released,
};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);
