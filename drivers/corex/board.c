/* SPDX-License-Identifier: MIT
 * R A14 uses two discrete, active-low RGB LEDs, not Cornix's WS2812 strip.
 */
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/conn.h>
#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/battery_state_changed.h>

static const struct gpio_dt_spec battery_enable = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio1)), .pin = 10,
};
static int battery_enable_init(void) {
    if (!gpio_is_ready_dt(&battery_enable)) { return -ENODEV; }
    int rc = gpio_pin_configure_dt(&battery_enable, GPIO_OUTPUT_ACTIVE);
    if (rc < 0) { return rc; }
    /* ZMK's divider driver only waits 10ms per sample. Keep this high so
     * R7/R8/C10 can settle for 350ms before the first battery report. */
    k_sleep(K_MSEC(350));
    return 0;
}
SYS_INIT(battery_enable_init, POST_KERNEL, 90);

#define LED(pin_) { .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)), \
                    .pin = pin_, .dt_flags = GPIO_ACTIVE_LOW }
/* R,G,B; source: CoreX-RMK right/src/main.rs, raytac-r-a14. */
static const struct gpio_dt_spec leds[2][3] = {
    {LED(16), LED(19), LED(25)}, {LED(20), LED(11), LED(23)},
};
static atomic_t ready, host_ticks, peer_ticks, low_ticks, peer_connected;
static void show(int index, int color) {
    for (int c = 0; c < 3; c++) { gpio_pin_set_dt(&leds[index][c], c == color); }
}
static void led_work(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(status_work, led_work);
static void led_work(struct k_work *work) {
    ARG_UNUSED(work);
    if (!atomic_get(&ready)) { return; }
    int host = atomic_get(&host_ticks), peer = atomic_get(&peer_ticks);
    int low = atomic_get(&low_ticks);
    int profile = zmk_ble_active_profile_index();
    /* Profile 0/1/2 = green/red/blue. Pulse briefly, then stay dark. */
    int color = profile == 0 ? 1 : profile == 1 ? 0 : 2;
    show(1, host > 0 && (host % 4 < 2) ? color : -1);
    show(0, low > 0 ? (low % 2 ? 0 : -1) :
            peer > 0 && (atomic_get(&peer_connected) || peer % 4 < 2) ? 2 : -1);
    if (host > 0) { atomic_dec(&host_ticks); }
    if (peer > 0) { atomic_dec(&peer_ticks); }
    if (low > 0) { atomic_dec(&low_ticks); }
    if (host > 0 || peer > 0 || low > 0) {
        k_work_reschedule(&status_work, K_MSEC(250));
    }
}
static int status_event(const zmk_event_t *eh) {
    if (as_zmk_ble_active_profile_changed(eh)) {
        atomic_set(&host_ticks, zmk_ble_active_profile_is_connected() ? 4 : 40);
    }
    const struct zmk_battery_state_changed *b = as_zmk_battery_state_changed(eh);
    if (b && b->state_of_charge > 0 && b->state_of_charge <= 20) {
        atomic_set(&low_ticks, 8);
    }
    if (atomic_get(&ready)) { k_work_reschedule(&status_work, K_NO_WAIT); }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(corex_status, status_event);
ZMK_SUBSCRIPTION(corex_status, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(corex_status, zmk_battery_state_changed);
static void peer_status(struct bt_conn *conn, bool connected) {
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) || info.role != BT_CONN_ROLE_CENTRAL) { return; }
    atomic_set(&peer_connected, connected);
    atomic_set(&peer_ticks, connected ? 12 : 40);
    if (atomic_get(&ready)) { k_work_reschedule(&status_work, K_NO_WAIT); }
}
static void connected(struct bt_conn *conn, uint8_t err) {
    if (!err) { peer_status(conn, true); }
}
static void disconnected(struct bt_conn *conn, uint8_t reason) {
    ARG_UNUSED(reason); peer_status(conn, false);
}
BT_CONN_CB_DEFINE(corex_conn) = {.connected = connected, .disconnected = disconnected};
static int status_init(void) {
    for (int i = 0; i < 2; i++) {
        for (int c = 0; c < 3; c++) {
            if (!gpio_is_ready_dt(&leds[i][c])) { return -ENODEV; }
            int rc = gpio_pin_configure_dt(&leds[i][c], GPIO_OUTPUT_INACTIVE);
            if (rc < 0) { return rc; }
        }
    }
    atomic_set(&ready, 1);
    atomic_set(&host_ticks, 40);
    atomic_set(&peer_ticks, 40);
    k_work_reschedule(&status_work, K_MSEC(500));
    return 0;
}
SYS_INIT(status_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
