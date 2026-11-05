/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2024 Google LLC; modifications 2025 sekigon-gonnoc.
 * Rust adaptation 2026 CoreX prototype; GPIO implementation 2026.
 * GPIO transport for the CoreX PAW3222; included by the driver and host tests.
 */
#pragma once

static inline void send_byte(const struct paw_config *c, uint8_t value) {
    gpio_pin_configure_dt(&c->sdio, GPIO_OUTPUT);
    for (int bit = 7; bit >= 0; bit--) {
        gpio_pin_set_dt(&c->sck, 0);
        gpio_pin_set_dt(&c->sdio, !!(value & BIT(bit)));
        k_busy_wait(1);
        gpio_pin_set_dt(&c->sck, 1);
        k_busy_wait(1);
    }
}
static inline int read_reg(const struct paw_config *c, uint8_t reg) {
    send_byte(c, reg & 0x7f);
    int rc = gpio_pin_configure_dt(&c->sdio, GPIO_INPUT);
    if (rc < 0) { return rc; }
    k_busy_wait(4);
    uint8_t value = 0;
    for (int bit = 0; bit < 8; bit++) {
        gpio_pin_set_dt(&c->sck, 0);
        k_busy_wait(1);
        gpio_pin_set_dt(&c->sck, 1);
        k_busy_wait(1);
        int sample = gpio_pin_get_dt(&c->sdio);
        if (sample < 0) { return sample; }
        value = (value << 1) | !!sample;
    }
    k_busy_wait(4);
    return value;
}
static inline void write_reg(const struct paw_config *c, uint8_t reg, uint8_t value) {
    send_byte(c, reg | 0x80);
    send_byte(c, value);
    gpio_pin_configure_dt(&c->sdio, GPIO_INPUT);
    k_busy_wait(4);
}
