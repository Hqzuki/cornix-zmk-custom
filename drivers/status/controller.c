/* SPDX-License-Identifier: MIT */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <hal/nrf_power.h>
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/ble.h>
#include <zmk/split/central.h>
#include <zmk/keymap.h>
#include <zmk/events/ble_active_profile_changed.h>
#else
#include <zmk/events/split_peripheral_status_changed.h>
#endif
#include "backend.h"
LOG_MODULE_REGISTER(corex_led, CONFIG_ZMK_LOG_LEVEL);
static struct status_state state = {
    .low_since = -1, .full_since = -1,
    .hide_peer = IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL),
    .layer_outer = !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL),
};
K_MUTEX_DEFINE(state_lock);
static bool ready;
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static int64_t next_sync;
#endif
/* VBUS detection also works with a charger or a charge-only cable, without
 * USB enumeration on the peripheral. Both supported boards use nRF52840. */
static bool usb_powered(void) {
    return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0;
}

int corex_status_receive(uint32_t profile, uint32_t flags) {
    k_mutex_lock(&state_lock, K_FOREVER);
    bool ok = status_receive(&state, profile, flags, k_uptime_get());
    k_mutex_unlock(&state_lock);
    return ok ? 0 : -EINVAL;
}

static void render(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(render_work, render);
static void render(struct k_work *work) {
    ARG_UNUSED(work);
    int64_t now = k_uptime_get();
    k_mutex_lock(&state_lock, K_FOREVER);
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    int profile = zmk_ble_active_profile_index();
    if (profile >= 0 && status_profile(&state, profile,
            zmk_ble_active_profile_is_connected(), zmk_ble_active_profile_is_open(), now)) {
        next_sync = 0;
    }
    uint8_t layer = zmk_keymap_layer_index_to_id(zmk_keymap_highest_layer_active());
    bool idle = zmk_activity_get_state() != ZMK_ACTIVITY_ACTIVE;
    if (!state.layer_valid || state.layer != layer || state.idle != idle) {
        status_layer(&state, true, layer);
        state.idle = idle;
        next_sync = 0; /* push layer and activity changes to the left immediately */
    }
#else
    state.idle = zmk_activity_get_state() != ZMK_ACTIVITY_ACTIVE && !state.remote_active;
#endif
    status_power(&state, usb_powered(), now);
    struct status_state snapshot = state;
    k_mutex_unlock(&state_lock);
    if (ready) { status_backend_write(status_render(&snapshot, now), now); }
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    /* Retry after discovery/encryption and resync after reconnect. The receiver
     * ignores unchanged state, so retries never restart a visible pattern. */
    if (snapshot.peer_connected && snapshot.profile_valid && now >= next_sync) {
        struct zmk_behavior_binding binding = {
            .behavior_dev = "ls", .param1 = snapshot.profile,
            .param2 = status_sync_flags(&snapshot),
        };
        struct zmk_behavior_binding_event event = {.timestamp = now};
        int rc = zmk_split_central_invoke_behavior(0, &binding, event, true);
        next_sync = now + (rc < 0 ? 500 : 2000);
    }
#endif
    k_work_reschedule(&render_work, K_MSEC(33));
}
static int status_event(const zmk_event_t *eh) {
    int64_t now = k_uptime_get();
    k_mutex_lock(&state_lock, K_FOREVER);
    const struct zmk_battery_state_changed *battery = as_zmk_battery_state_changed(eh);
    if (battery) { status_battery(&state, battery->state_of_charge, now); }
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    const struct zmk_split_peripheral_status_changed *peer = as_zmk_split_peripheral_status_changed(eh);
    if (peer) {
        status_peer(&state, peer->connected, now);
        if (!peer->connected) {
            state.profile_valid = false;
            state.remote_active = false;
            status_layer(&state, false, 0);
        }
    }
#endif
    k_mutex_unlock(&state_lock);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(corex_led_status, status_event);
ZMK_SUBSCRIPTION(corex_led_status, zmk_battery_state_changed);
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
ZMK_SUBSCRIPTION(corex_led_status, zmk_split_peripheral_status_changed);
#else
static void peer_status(struct bt_conn *conn, bool connected) {
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) || info.role != BT_CONN_ROLE_CENTRAL) { return; }
    k_mutex_lock(&state_lock, K_FOREVER);
    status_peer(&state, connected, k_uptime_get());
    k_mutex_unlock(&state_lock);
}
static void peer_connected(struct bt_conn *conn, uint8_t err) {
    if (!err) { peer_status(conn, true); }
}
static void peer_disconnected(struct bt_conn *conn, uint8_t reason) {
    ARG_UNUSED(reason); peer_status(conn, false);
}
BT_CONN_CB_DEFINE(corex_led_peer) = {.connected = peer_connected, .disconnected = peer_disconnected};
#endif
static int status_init(void) {
    int rc = status_backend_init();
    ready = rc == 0;
    if (rc < 0) { LOG_ERR("LED backend unavailable: %d", rc); }
    k_mutex_lock(&state_lock, K_FOREVER);
    if (!state.peer_known) { status_peer(&state, false, k_uptime_get()); }
    status_power(&state, usb_powered(), k_uptime_get());
    k_mutex_unlock(&state_lock);
    k_work_reschedule(&render_work, K_MSEC(500));
    return 0;
}
SYS_INIT(status_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
