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
