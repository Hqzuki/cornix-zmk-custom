/* SPDX-License-Identifier: MIT
 * Uses ZMK's existing encrypted split behavior channel; no extra BLE link.
 */
#define DT_DRV_COMPAT corex_led_sync
#include <errno.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include "backend.h"
static int sync_pressed(struct zmk_behavior_binding *binding,
                        struct zmk_behavior_binding_event event) {
    (void)event;
#if IS_ENABLED(CONFIG_COREX_LED_STATUS) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (binding->param1 > 255 || (binding->param2 & ~3u)) { return -EINVAL; }
    corex_status_receive_profile(binding->param1, binding->param2);
    return ZMK_BEHAVIOR_OPAQUE;
#else
    (void)binding;
    return -ENOTSUP;
#endif
}
static int sync_released(struct zmk_behavior_binding *binding,
                         struct zmk_behavior_binding_event event) {
    (void)binding; (void)event; return ZMK_BEHAVIOR_OPAQUE;
}
static const struct behavior_driver_api sync_api = {
    .binding_pressed = sync_pressed, .binding_released = sync_released,
};
BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &sync_api);
