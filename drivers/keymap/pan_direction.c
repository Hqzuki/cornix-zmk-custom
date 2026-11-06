/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT custom_input_processor_pan_direction

#include <drivers/input_processor.h>
#include <zmk/keymap.h>

/* Run before inertia so both live and inertial scroll use the same direction. */
static int pan_direction_handle(const struct device *dev, struct input_event *event,
                                uint32_t param1, uint32_t param2,
                                struct zmk_input_processor_state *state) {
    const uint8_t *mac_layer = dev->config;
    if (event->type == INPUT_EV_REL && event->code == INPUT_REL_HWHEEL &&
        zmk_keymap_layer_active(zmk_keymap_layer_index_to_id(*mac_layer))) {
        event->value = -event->value;
    }
    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api pan_direction_api = {
    .handle_event = pan_direction_handle,
};

#define PAN_DIRECTION_INST(n)                                                               \
    static const uint8_t mac_layer_##n = DT_INST_PROP(n, mac_layer);                         \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &mac_layer_##n, POST_KERNEL,                   \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &pan_direction_api);

DT_INST_FOREACH_STATUS_OKAY(PAN_DIRECTION_INST)
