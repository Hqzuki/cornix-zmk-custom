/* SPDX-License-Identifier: MIT
 * R A14: two active-low RGB LEDs, hardware PWM for the Cornix breathing fade.
 */
#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include "backend.h"
static const struct pwm_dt_spec leds[2][3] = {
    {PWM_DT_SPEC_GET(DT_NODELABEL(corex_inner_r)),
     PWM_DT_SPEC_GET(DT_NODELABEL(corex_inner_g)),
     PWM_DT_SPEC_GET(DT_NODELABEL(corex_inner_b))},
    {PWM_DT_SPEC_GET(DT_NODELABEL(corex_outer_r)),
     PWM_DT_SPEC_GET(DT_NODELABEL(corex_outer_g)),
     PWM_DT_SPEC_GET(DT_NODELABEL(corex_outer_b))},
};
int status_backend_write(struct status_frame frame, int64_t now) {
    (void)now;
    const struct status_rgb colors[] = {frame.inner, frame.outer};
    for (int i = 0; i < 2; i++) {
        uint8_t values[] = {colors[i].r, colors[i].g, colors[i].b};
        for (int c = 0; c < 3; c++) {
            int rc = pwm_set_pulse_dt(&leds[i][c], leds[i][c].period * values[c] / 255);
            if (rc < 0) { return rc; }
        }
    }
    return 0;
}
int status_backend_init(void) {
    for (int i = 0; i < 2; i++) {
        for (int c = 0; c < 3; c++) {
            if (!pwm_is_ready_dt(&leds[i][c])) { return -ENODEV; }
        }
    }
    return status_backend_write((struct status_frame){0}, 0);
}
