/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* Adapted from cormoran/zmk-feature-default-layer at
 * 5e3bf04e519160ac2fb583095b2ec92e9f9f4b1d for current ZMK APIs.
 */
#define DT_DRV_COMPAT zmk_behavior_default_layer

#include <errno.h>
#include <drivers/behavior.h>
#include <dt-bindings/zmk_behavior_default_layer/default_layer.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/ble.h>
#include "endpoint_policy.h"
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct default_layer_settings_t {
    uint8_t endpoint_defaults[ZMK_ENDPOINT_COUNT];
};

static struct default_layer_settings_t default_layers = {0};

/* Resolve after BT_SEL from the active profile directly: the selected
 * endpoint may still reference the previous link or report NONE until paired.
 * USB remains independent when it is the actual output transport. */
static struct zmk_endpoint_instance layer_endpoint(bool explicit_ble) {
    struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();
    int slot = default_layer_target_slot(selected.transport == ZMK_TRANSPORT_USB,
                                         zmk_ble_active_profile_index(), explicit_ble);
    if (slot < 0) { return (struct zmk_endpoint_instance){.transport = ZMK_TRANSPORT_NONE}; }
    if (slot == 0) { return (struct zmk_endpoint_instance){.transport = ZMK_TRANSPORT_USB}; }
    return (struct zmk_endpoint_instance){.transport = ZMK_TRANSPORT_BLE,
                                         .ble.profile_index = slot - 1};
}

static int layer_endpoint_index(struct zmk_endpoint_instance endpoint) {
    if (endpoint.transport == ZMK_TRANSPORT_NONE) { return -ENODEV; }
    if (endpoint.transport == ZMK_TRANSPORT_BLE &&
        (endpoint.ble.profile_index < 0 || endpoint.ble.profile_index >= ZMK_BLE_PROFILE_COUNT)) {
        return -EINVAL;
    }
    int index = zmk_endpoint_instance_to_index(endpoint);
    return index >= 0 && index < ARRAY_SIZE(default_layers.endpoint_defaults) ? index : -EINVAL;
}

static zmk_keymap_layer_id_t zmk_default_layer_get(struct zmk_endpoint_instance endpoint) {
    int index = layer_endpoint_index(endpoint);
    return index < 0 ? CONFIG_ZMK_DEFAULT_LAYER_MIN_INDEX : default_layers.endpoint_defaults[index];
}

static int apply_default_layer_config(struct zmk_endpoint_instance endpoint) {
    if (layer_endpoint_index(endpoint) < 0) { return 0; }
    int layer_index = zmk_default_layer_get(endpoint);
    if (layer_index < CONFIG_ZMK_DEFAULT_LAYER_MIN_INDEX ||
        layer_index > CONFIG_ZMK_DEFAULT_LAYER_MAX_INDEX || layer_index >= ZMK_KEYMAP_LAYERS_LEN) {
        return -EINVAL;
    }
    // deactivate other layers first
    zmk_keymap_layer_id_t global_default_layer_index =
        zmk_keymap_layer_default();  // TODO: zmk_keymap_layer_id_to_index()?
    for (int i = CONFIG_ZMK_DEFAULT_LAYER_MIN_INDEX;
         i <= CONFIG_ZMK_DEFAULT_LAYER_MAX_INDEX; i++) {
        if (i != layer_index && i != global_default_layer_index) {
            int rc =
                zmk_keymap_layer_deactivate(zmk_keymap_layer_index_to_id(i), false);
            if (rc != 0) {
                LOG_WRN("Failed deactivate layer: %d for endpoint %d",
                        default_layers.endpoint_defaults[i], i);
            }
        }
    }
    if (layer_index != global_default_layer_index) {
        int ret = zmk_keymap_layer_activate(
            zmk_keymap_layer_index_to_id(layer_index), false);
        if (ret < 0) {
            LOG_WRN(
                "Could not apply default layer from settings. Perhaps "
                "something in "
                "the code/keymap changed since configuration was saved.");
            return ret;
        }

        LOG_INF("Activated default layer (%d) for the current endpoint.",
                layer_index);
    }
    return 0;
}

static int default_layer_set(const char *name, size_t len,
                             settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    int rc;

    if (settings_name_steq(name, "settings", &next) && !next) {
        if (len != sizeof(default_layers)) {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &default_layers, sizeof(default_layers));
        if (rc >= 0) {
            return 0;
        }

        return rc;
    }

    return -ENOENT;
}

struct settings_handler default_layer_conf = {
    .name  = "default_layer",
    .h_set = default_layer_set,
};

static int default_layer_init(void) {
    settings_subsys_init();

    int ret = settings_register(&default_layer_conf);
    if (ret) {
        LOG_ERR("Could not register default layer settings (%d).", ret);
        return ret;
    }

    settings_load_subtree("default_layer");

    /* Endpoint events will reapply after transport initialization. */
    return apply_default_layer_config(layer_endpoint(false));
}
SYS_INIT(default_layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int zmk_default_layer_set(struct zmk_endpoint_instance endpoint,
                                 zmk_keymap_layer_id_t layer) {
    if (layer >= ZMK_KEYMAP_LAYERS_LEN) {
        return -EINVAL;
    }
    if (layer < CONFIG_ZMK_DEFAULT_LAYER_MIN_INDEX ||
        CONFIG_ZMK_DEFAULT_LAYER_MAX_INDEX < layer) {
        return -EINVAL;
    }

    int index = layer_endpoint_index(endpoint);
    if (index < 0) { return index; }
    uint8_t previous = default_layers.endpoint_defaults[index];
    if (previous != layer) {
        default_layers.endpoint_defaults[index] = layer;
        /* Layout selection is rare and must survive switching power off
         * immediately. ZMK's general settings debounce defaults to 60 s.
         * Save before applying; an unchanged selection needs no flash write. */
        int ret = settings_save_one("default_layer/settings", &default_layers,
                                    sizeof(default_layers));
        if (ret < 0) {
            default_layers.endpoint_defaults[index] = previous;
            LOG_ERR("Could not save default layer (%d).", ret);
            return ret;
        }
    }

    char endpoint_str[ZMK_ENDPOINT_STR_LEN];
    zmk_endpoint_instance_to_str(endpoint, endpoint_str, sizeof(endpoint_str));
    LOG_INF("Updated default layer (%d) for %s.", layer, endpoint_str);

    /* Saving a BLE profile while USB is active must not change USB's layout. */
    if (zmk_endpoint_instance_eq(endpoint, layer_endpoint(false))) {
        return apply_default_layer_config(endpoint);
    }
    return 0;
}

static int behavior_default_layer_init(const struct device *dev) { return 0; }

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    struct zmk_endpoint_instance endpoint =
        layer_endpoint(binding->param1 == DEFAULT_LAYER_CMD_SELECT_BLE);

    switch (binding->param1) {
        case DEFAULT_LAYER_CMD_SELECT:
        case DEFAULT_LAYER_CMD_SELECT_BLE:
            return zmk_default_layer_set(endpoint, binding->param2);
            break;
        case DEFAULT_LAYER_CMD_NEXT: {
            zmk_keymap_layer_id_t current = zmk_default_layer_get(endpoint);
            zmk_keymap_layer_id_t next =
                current >= CONFIG_ZMK_DEFAULT_LAYER_MAX_INDEX
                    ? CONFIG_ZMK_DEFAULT_LAYER_MIN_INDEX
                    : current + 1;
            return zmk_default_layer_set(endpoint, next);
        }
        default:
            LOG_ERR("Unknown command for df: %d", binding->param1);
    }
    return 0;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata select_param1_values[] = {
    {
        .display_name = "Select default layer for current transport",
        .type         = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value        = DEFAULT_LAYER_CMD_SELECT,
    },
};

static const struct behavior_parameter_value_metadata ble_select_param1_values[] = {
    {.display_name = "Select default layer for active Bluetooth profile",
     .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE, .value = DEFAULT_LAYER_CMD_SELECT_BLE},
};

static const struct behavior_parameter_value_metadata select_param2_values[] = {
    {
        .display_name = "Animation index",
        .type         = BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,
        .range =
            {
                .min = CONFIG_ZMK_DEFAULT_LAYER_MIN_INDEX,
                .max = CONFIG_ZMK_DEFAULT_LAYER_MAX_INDEX,
            },
    },
};

static const struct behavior_parameter_metadata_set select_metadata_set = {
    .param1_values     = select_param1_values,
    .param1_values_len = ARRAY_SIZE(select_param1_values),
    .param2_values     = select_param2_values,
    .param2_values_len = ARRAY_SIZE(select_param2_values),
};

static const struct behavior_parameter_value_metadata next_param1_values[] = {
    {
        .display_name = "Select next default layer for current transport",
        .type         = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
        .value        = DEFAULT_LAYER_CMD_NEXT,
    },
};

static const struct behavior_parameter_metadata_set next_metadata_set = {
    .param1_values     = next_param1_values,
    .param1_values_len = ARRAY_SIZE(next_param1_values),
};

static const struct behavior_parameter_metadata_set metadata_sets[] = {
    select_metadata_set, next_metadata_set,
    {.param1_values = ble_select_param1_values, .param1_values_len = ARRAY_SIZE(ble_select_param1_values),
     .param2_values = select_param2_values, .param2_values_len = ARRAY_SIZE(select_param2_values)}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(metadata_sets),
    .sets     = metadata_sets,
};

#endif

static const struct behavior_driver_api behavior_default_layer_driver_api = {
    .binding_pressed  = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif  // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_default_layer_init, NULL, NULL, NULL,
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_default_layer_driver_api);

static int endpoint_changed_cb(const zmk_event_t *eh) {
    if (as_zmk_ble_active_profile_changed(eh) || as_zmk_endpoint_changed(eh)) {
        apply_default_layer_config(layer_endpoint(false));
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(default_layer, endpoint_changed_cb);
ZMK_SUBSCRIPTION(default_layer, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(default_layer, zmk_ble_active_profile_changed);

#endif
