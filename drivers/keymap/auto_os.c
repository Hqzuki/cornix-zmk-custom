/* SPDX-License-Identifier: MIT
 * BLE read-pattern approach: cormoran/zmk-feature-os-detection,
 * src/os_detection_ble.c blob 71014c18274f10023b6b5878db97c2bd6b9b958f.
 * See auto_os.LICENSE for attribution. No settings, USB hooks or custom RPC.
 */
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum evidence {
    REPORT_MAP = BIT(0), HID_INFO = BIT(1), PNP_ID = BIT(2), APPEARANCE = BIT(3),
};
enum host_os { OS_UNKNOWN, OS_WIN, OS_APPLE };

/* These are stable keymap IDs, not Studio display-order indices. */
#define MAC_LAYER_ID 1

struct host_session {
    struct bt_conn *conn; /* retained only until disconnect */
    uint8_t reads;
    bool settled;
    enum host_os os;
    int64_t deadline;
};
static struct host_session sessions[CONFIG_BT_MAX_CONN];
static struct k_spinlock session_lock;

static enum host_os classify(uint8_t reads) {
    if ((reads & (PNP_ID | APPEARANCE)) == (PNP_ID | APPEARANCE)) {
        return OS_WIN; /* Also Linux/BlueZ: indistinguishable with this signal. */
    }
    if ((reads & (REPORT_MAP | HID_INFO)) == (REPORT_MAP | HID_INFO)) {
        return OS_WIN; /* Android uses the same base modifier layout. */
    }
    if (reads & (REPORT_MAP | PNP_ID)) {
        /* Recorded macOS capture read only DIS PnP ID (no Report Map).
         * iOS read only Report Map. Evaluate these after the WIN rules. */
        return OS_APPLE;
    }
    return OS_UNKNOWN;
}

static void update_layout(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(update_work, update_layout);
static void request_update(void) { k_work_reschedule(&update_work, K_NO_WAIT); }

static void update_layout(struct k_work *work) {
    ARG_UNUSED(work);
    int64_t now = k_uptime_get();
    int64_t next = INT64_MAX;
    enum host_os os = OS_UNKNOWN;
    uint8_t finalized[CONFIG_BT_MAX_CONN] = {0};
    /* ZMK returns a reference, including when this is not the active endpoint. */
    struct bt_conn *active = zmk_ble_active_profile_conn();
    struct zmk_endpoint_instance endpoint = zmk_endpoint_get_selected();
    k_spinlock_key_t key = k_spin_lock(&session_lock);
    for (size_t i = 0; i < ARRAY_SIZE(sessions); i++) {
        struct host_session *s = &sessions[i];
        if (!s->conn) { continue; }
        if (s->reads && !s->settled) {
            if (now >= s->deadline) {
                s->settled = true;
                s->os = classify(s->reads);
                finalized[i] = s->reads;
            } else if (s->deadline < next) {
                next = s->deadline;
            }
        }
        if (s->conn == active &&
            endpoint.transport == ZMK_TRANSPORT_BLE) {
            os = s->os;
        }
    }
    k_spin_unlock(&session_lock, key);
    if (active) { bt_conn_unref(active); }
    for (size_t i = 0; i < ARRAY_SIZE(finalized); i++) {
        if (finalized[i]) {
            LOG_INF("Bluetooth OS evidence: conn=%u reads=0x%02x result=%d",
                    (unsigned)i, (unsigned)finalized[i], classify(finalized[i]));
        }
    }

    /* WIN until there is a settled Apple fingerprint. Never save a guess. */
    bool mac = os == OS_APPLE;
    if (zmk_keymap_layer_active(MAC_LAYER_ID) != mac) {
        int rc = mac ? zmk_keymap_layer_activate(MAC_LAYER_ID, false)
                     : zmk_keymap_layer_deactivate(MAC_LAYER_ID, false);
        if (rc < 0) {
            LOG_ERR("Could not apply automatic host layout (%d)", rc);
        } else {
            LOG_INF("Automatic Bluetooth layout: %s", mac ? "Apple" : "WIN");
        }
    }
    /* Preserve any immediate request queued by a callback during this run. */
    if (next != INT64_MAX) {
        k_work_schedule(&update_work, K_MSEC(MAX(0, next - k_uptime_get())));
    }
}

static void host_connected(struct bt_conn *conn, uint8_t err) {
    struct bt_conn_info info;
    if (err || bt_conn_get_info(conn, &info) || info.type != BT_CONN_TYPE_LE ||
        info.role != BT_CONN_ROLE_PERIPHERAL) { return; }
    /* The split peer has the opposite role and must never affect host OS. */
    uint8_t index = bt_conn_index(conn);
    if (index >= ARRAY_SIZE(sessions)) { return; }
    struct bt_conn *ref = bt_conn_ref(conn);
    k_spinlock_key_t key = k_spin_lock(&session_lock);
    struct bt_conn *old = sessions[index].conn;
    sessions[index] = (struct host_session){.conn = ref};
    k_spin_unlock(&session_lock, key);
    if (old) { bt_conn_unref(old); }
    request_update();
}

static void host_disconnected(struct bt_conn *conn, uint8_t reason) {
    ARG_UNUSED(reason);
    uint8_t index = bt_conn_index(conn);
    if (index >= ARRAY_SIZE(sessions)) { return; }
    k_spinlock_key_t key = k_spin_lock(&session_lock);
    struct bt_conn *old = NULL;
    if (sessions[index].conn == conn) {
        old = sessions[index].conn;
        sessions[index] = (struct host_session){0};
    }
    k_spin_unlock(&session_lock, key);
    if (old) { bt_conn_unref(old); request_update(); }
}

static bool observe_read(struct bt_conn *conn, const struct bt_gatt_attr *attr) {
    if (!conn) { return true; }
    uint8_t bit;
    if (!bt_uuid_cmp(attr->uuid, BT_UUID_HIDS_REPORT_MAP)) { bit = REPORT_MAP; }
    else if (!bt_uuid_cmp(attr->uuid, BT_UUID_HIDS_INFO)) { bit = HID_INFO; }
    else if (!bt_uuid_cmp(attr->uuid, BT_UUID_DIS_PNP_ID)) { bit = PNP_ID; }
    else if (!bt_uuid_cmp(attr->uuid, BT_UUID_GAP_APPEARANCE)) { bit = APPEARANCE; }
    else { return true; } /* Battery polls and report reads cannot delay settling. */
    uint8_t index = bt_conn_index(conn);
    if (index >= ARRAY_SIZE(sessions)) { return true; }
    bool changed = false;
    k_spinlock_key_t key = k_spin_lock(&session_lock);
    struct host_session *s = &sessions[index];
    if (s->conn == conn && !(s->reads & bit)) {
        s->reads |= bit;
        /* Keep observing throughout the connection. Late discovery must be
         * able to refine a guess, without repeated polls restarting it. */
        s->settled = false;
        s->deadline = k_uptime_get() + CONFIG_CORNIX_AUTO_OS_SETTLE_MS;
        changed = true;
    }
    k_spin_unlock(&session_lock, key);
    if (changed) { request_update(); }
    return true; /* Observe only; never reject a read. */
}

static struct bt_gatt_authorization_cb authorization = {.read_authorize = observe_read};
BT_CONN_CB_DEFINE(auto_os_connections) = {
    .connected = host_connected,
    .disconnected = host_disconnected,
};

static int auto_os_init(void) {
    int rc = bt_gatt_authorization_cb_register(&authorization);
    if (rc < 0) { LOG_ERR("Could not register host OS observer (%d)", rc); }
    return rc;
}
SYS_INIT(auto_os_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int host_changed(const zmk_event_t *event) {
    ARG_UNUSED(event);
    request_update();
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(auto_os, host_changed);
ZMK_SUBSCRIPTION(auto_os, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(auto_os, zmk_endpoint_changed);
