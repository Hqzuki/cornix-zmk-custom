/* SPDX-License-Identifier: MIT
 * Cornix left: two WS2812 pixels, index 0 inner / 1 outer.
 * Rail gating follows the original Cornix widget (2026 numachang).
 */
#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <drivers/ext_power.h>
#include "backend.h"
static const struct device *const strip = DEVICE_DT_GET(DT_ALIAS(status_ws2812));
static const struct device *const power = DEVICE_DT_GET(DT_INST(0, zmk_ext_power_generic));
static int64_t last_lit;
static bool have_frame;
static struct status_frame previous;
BUILD_ASSERT(DT_PROP(DT_ALIAS(status_ws2812), chain_length) == 2);
int status_backend_init(void) {
    return device_is_ready(strip) && device_is_ready(power) ? 0 : -ENODEV;
}
int status_backend_write(struct status_frame frame, int64_t now) {
    bool lit = frame.inner.r || frame.inner.g || frame.inner.b ||
               frame.outer.r || frame.outer.g || frame.outer.b;
    if (lit) { last_lit = now; }
    int powered = ext_power_get(power);
    if (powered < 0) { return powered; }
    if (!powered && !lit) { return 0; }
    if (!powered) {
        int rc = ext_power_enable(power);
        if (rc < 0) { return rc; }
        k_sleep(K_MSEC(10)); have_frame = false;
    }
    if (!have_frame || memcmp(&previous, &frame, sizeof(frame))) {
        struct led_rgb pixels[] = {
            {.r=frame.inner.r, .g=frame.inner.g, .b=frame.inner.b},
            {.r=frame.outer.r, .g=frame.outer.g, .b=frame.outer.b},
        };
        int rc = led_strip_update_rgb(strip, pixels, 2);
        if (rc < 0) { return rc; }
        previous = frame; have_frame = true;
    }
    if (!lit && now - last_lit >= 15000) { return ext_power_disable(power); }
    return 0;
}
