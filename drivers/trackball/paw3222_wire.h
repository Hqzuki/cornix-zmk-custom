/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2024 Google LLC; modifications 2025 sekigon-gonnoc.
 * Cornix GPIO integration.
 * GPIO half-duplex PAW3222 transport, shared with the host tests.
 */
#pragma once

/* NCS is active low. The additional 1 us NCS setup/hold gaps remain provisional
 * pending hardware validation. Open-drain SDIO limits contention during probing.
 */
static inline int cs_assert(const struct paw_config *c) {
    if (!(c->auxiliary.dt_flags & GPIO_ACTIVE_LOW)) { return 0; }
    int rc = gpio_pin_set_dt(&c->auxiliary, 1);
    if (rc < 0) { return rc; }
    k_busy_wait(1);
    return 0;
}
static inline int cs_release(const struct paw_config *c) {
    if (!(c->auxiliary.dt_flags & GPIO_ACTIVE_LOW)) { return 0; }
    k_busy_wait(1);
    int rc = gpio_pin_set_dt(&c->auxiliary, 0);
    k_busy_wait(1);
    return rc;
}
static inline int send_byte(const struct paw_config *c, uint8_t value) {
    int flags = GPIO_OUTPUT | GPIO_PULL_UP;
    if (c->probing) { flags |= GPIO_OPEN_DRAIN; }
    int rc = gpio_pin_configure_dt(&c->sdio, flags);
    if (rc < 0) { return rc; }
    for (int bit = 7; bit >= 0; bit--) {
        if ((rc = gpio_pin_set_dt(&c->sck, 0)) < 0 ||
            (rc = gpio_pin_set_dt(&c->sdio, !!(value & BIT(bit)))) < 0) { return rc; }
        k_busy_wait(1);
        if ((rc = gpio_pin_set_dt(&c->sck, 1)) < 0) { return rc; }
        k_busy_wait(1);
    }
    return 0;
}
/* One address/data exchange, while the caller holds NCS active. */
static inline int read_reg_raw(const struct paw_config *c, uint8_t reg) {
    int rc = send_byte(c, reg & 0x7f);
    if (rc < 0) { return rc; }
    rc = gpio_pin_configure_dt(&c->sdio, GPIO_INPUT | GPIO_PULL_UP);
    if (rc < 0) { return rc; }
    k_busy_wait(4);
    uint8_t value = 0;
    for (int bit = 0; bit < 8; bit++) {
        if ((rc = gpio_pin_set_dt(&c->sck, 0)) < 0) { return rc; }
        k_busy_wait(1);
        if ((rc = gpio_pin_set_dt(&c->sck, 1)) < 0) { return rc; }
        k_busy_wait(1);
        int sample = gpio_pin_get_dt(&c->sdio);
        if (sample < 0) { return sample; }
        value = (value << 1) | !!sample;
    }
    k_busy_wait(4);
    return value;
}
/* Even on a GPIO failure, release SDIO and NCS and restore idle-high SCK. */
static inline int finish_transaction(const struct paw_config *c, int result) {
    int rc = gpio_pin_configure_dt(&c->sdio, GPIO_INPUT | GPIO_PULL_UP);
    if (result >= 0 && rc < 0) { result = rc; }
    rc = gpio_pin_set_dt(&c->sck, 1);
    if (result >= 0 && rc < 0) { result = rc; }
    rc = cs_release(c);
    return result < 0 ? result : rc < 0 ? rc : result;
}
static inline int read_reg(const struct paw_config *c, uint8_t reg) {
    int rc = cs_assert(c);
    if (rc >= 0) { rc = read_reg_raw(c, reg); }
    return finish_transaction(c, rc);
}
static inline int write_reg(const struct paw_config *c, uint8_t reg, uint8_t value) {
    int rc = cs_assert(c);
    if (rc >= 0) { rc = send_byte(c, reg | 0x80); }
    if (rc >= 0) { rc = send_byte(c, value); }
    k_busy_wait(4);
    return finish_transaction(c, rc);
}
/* Same address/X/address/Y framing as sekigon-gonnoc's PAW3222 driver.
 * Keep NCS active across both reads; publish neither axis on partial failure.
 */
static inline int read_xy(const struct paw_config *c, int *x, int *y) {
    int rc = cs_assert(c), rx = -1, ry = -1;
    if (rc >= 0) { rx = rc = read_reg_raw(c, 0x03); }
    if (rc >= 0) { ry = rc = read_reg_raw(c, 0x04); }
    rc = finish_transaction(c, rc < 0 ? rc : 0);
    if (rc < 0) { return rc; }
    *x = rx; *y = ry;
    return 0;
}
