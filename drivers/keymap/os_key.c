/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT custom_behavior_os_key

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

struct os_key_config {
    uint32_t win_keycode;
    uint32_t mac_keycode;
    uint8_t mac_layer;
};

struct os_key_data {
    uint32_t pressed_keycode;
};

static int os_key_press(struct zmk_behavior_binding *binding,
                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct os_key_config *cfg = dev->config;
    struct os_key_data *data = dev->data;
    data->pressed_keycode =
        zmk_keymap_layer_active(zmk_keymap_layer_index_to_id(cfg->mac_layer))
            ? cfg->mac_keycode : cfg->win_keycode;
    struct zmk_behavior_binding key = {.behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(kp)), .param1 = data->pressed_keycode};
    return zmk_behavior_invoke_binding(&key, event, true);
}

static int os_key_release(struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct os_key_data *data = dev->data;
    /* Release the same key even if the base layer changed while held. */
    struct zmk_behavior_binding key = {.behavior_dev = DEVICE_DT_NAME(DT_NODELABEL(kp)), .param1 = data->pressed_keycode};
    return zmk_behavior_invoke_binding(&key, event, false);
}

static const struct behavior_driver_api os_key_api = {
    .binding_pressed = os_key_press,
    .binding_released = os_key_release,
};

#define OS_KEY_INST(n)                                                                           \
    static const struct os_key_config os_key_config_##n = {                                     \
        .win_keycode = DT_INST_PROP(n, win_keycode),                                              \
        .mac_keycode = DT_INST_PROP(n, mac_keycode),                                              \
        .mac_layer = DT_INST_PROP(n, mac_layer),                                                 \
    };                                                                                           \
    static struct os_key_data os_key_data_##n;                                                   \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &os_key_data_##n, &os_key_config_##n, POST_KERNEL,      \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &os_key_api);

DT_INST_FOREACH_STATUS_OKAY(OS_KEY_INST)
