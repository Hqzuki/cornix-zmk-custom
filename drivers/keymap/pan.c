/* SPDX-License-Identifier: MIT
 * Choose the CoronLP scroll layer using the active WIN/MAC base layer.
 * This also permits J+K while the automatic mouse layer is active.
 */
#define DT_DRV_COMPAT corex_coron_pan
#include <drivers/behavior.h>
#include <zmk/keymap.h>
static uint8_t held_layer;
static int pressed(struct zmk_behavior_binding *binding,
                   struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding); ARG_UNUSED(event);
    held_layer = zmk_keymap_layer_active(1) ? 4 : 3;
    return zmk_keymap_layer_activate(held_layer, false);
}
static int released(struct zmk_behavior_binding *binding,
                    struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding); ARG_UNUSED(event);
    return zmk_keymap_layer_deactivate(held_layer, false);
}
static const struct behavior_driver_api api = {
    .binding_pressed = pressed, .binding_released = released,
};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);
