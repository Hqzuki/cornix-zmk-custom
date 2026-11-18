/* SPDX-License-Identifier: MIT
 * The XIAO central has no Cornix WS2812 strip. Its onboard RGB LED is used
 * for the inner status color (battery / charging / low battery).
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include "backend.h"

static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

static void set_led(const struct gpio_dt_spec *led, uint8_t value) {
    /* XIAO's onboard RGB LED is active-low; any non-zero status value means on. */
    gpio_pin_set_dt(led, value ? 0 : 1);
}

int status_backend_init(void) {
    int rc = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    if (rc < 0) { return rc; }
    rc = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    if (rc < 0) { return rc; }
    return gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
}

int status_backend_write(struct status_frame frame, int64_t now) {
    (void)now;
    /* The central's useful local indication is the inner status color. */
    set_led(&led_red, frame.inner.r);
    set_led(&led_green, frame.inner.g);
    set_led(&led_blue, frame.inner.b);
    return 0;
}
