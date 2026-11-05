/* SPDX-License-Identifier: Apache-2.0
 * CoreX three-wire transport follows CoreX-RMK paw_wire.rs / paw3222.rs.
 * Register protocol: Copyright 2024 Google LLC, modifications 2025 sekigon-gonnoc,
 * Rust adaptation 2026 CoreX prototype. ZMK GPIO implementation 2026.
 * SDIO MUST be released while the sensor responds; SPIM with MOSI and MISO
 * on the same pin does not provide that guarantee.
 */
#define DT_DRV_COMPAT corex_paw3222_gpio
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(corex_paw, CONFIG_ZMK_LOG_LEVEL);

struct paw_config {
    struct gpio_dt_spec sck, sdio, motion, auxiliary;
    uint16_t cpi, interval;
};
struct paw_data {
    const struct device *dev;
    struct gpio_callback irq;
    struct k_work_delayable work;
    struct k_work_q queue;
    bool ready;
};
/* Exactly one connector per image, so an empty connector cannot disturb input. */
K_THREAD_STACK_DEFINE(paw_stack, 1536);

static void send_byte(const struct paw_config *c, uint8_t value) {
    gpio_pin_configure_dt(&c->sdio, GPIO_OUTPUT);
    for (int bit = 7; bit >= 0; bit--) {
        gpio_pin_set_dt(&c->sck, 0);
        gpio_pin_set_dt(&c->sdio, !!(value & BIT(bit)));
        k_busy_wait(1);
        gpio_pin_set_dt(&c->sck, 1);
        k_busy_wait(1);
    }
}
static int read_reg(const struct paw_config *c, uint8_t reg) {
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
static void write_reg(const struct paw_config *c, uint8_t reg, uint8_t value) {
    send_byte(c, reg | 0x80);
    send_byte(c, value);
    gpio_pin_configure_dt(&c->sdio, GPIO_INPUT);
    k_busy_wait(4);
}
static bool configure(const struct paw_config *c) {
    if (read_reg(c, 0x00) != 0x30) { return false; }
    int cfg = read_reg(c, 0x06);
    if (cfg < 0) { return false; }
    write_reg(c, 0x06, cfg | 0x80);
    k_sleep(K_MSEC(2));
    write_reg(c, 0x09, 0x5a);
    int op = read_reg(c, 0x05);
    if (op < 0) { write_reg(c, 0x09, 0); return false; }
    write_reg(c, 0x05, op | 0x18); /* Retain the sensor's low-power rest modes. */
    write_reg(c, 0x0d, c->cpi / 38);
    write_reg(c, 0x0e, c->cpi / 38);
    write_reg(c, 0x09, 0);
    read_reg(c, 0x02); read_reg(c, 0x03); read_reg(c, 0x04); read_reg(c, 0x12);
    return read_reg(c, 0x00) == 0x30;
}
static void schedule(struct paw_data *d, k_timeout_t delay) {
    k_work_reschedule_for_queue(&d->queue, &d->work, delay);
}
static void irq_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    ARG_UNUSED(port); ARG_UNUSED(pins);
    struct paw_data *d = CONTAINER_OF(cb, struct paw_data, irq);
    const struct paw_config *c = d->dev->config;
    gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_DISABLE);
    schedule(d, K_NO_WAIT);
}
static void sample_work(struct k_work *work) {
    struct paw_data *d = CONTAINER_OF(k_work_delayable_from_work(work), struct paw_data, work);
    const struct paw_config *c = d->dev->config;
    if (!d->ready) {
        if (!configure(c)) {
            /* Wrong/unplugged connector: retry quietly, never emit garbage. */
            schedule(d, K_SECONDS(2));
            return;
        }
        d->ready = true;
        LOG_INF("PAW3222 ready (%u CPI)", c->cpi);
    }
    int status = read_reg(c, 0x02);
    if (status < 0 || (status == 0xff && read_reg(c, 0x00) != 0x30)) {
        d->ready = false;
        schedule(d, K_SECONDS(2));
        return;
    }
    if (status & BIT(7)) {
        int x = read_reg(c, 0x03), y = read_reg(c, 0x04);
        if (x >= 0 && y >= 0 && (x || y)) {
            input_report_rel(d->dev, INPUT_REL_X, (int8_t)x, false, K_FOREVER);
            input_report_rel(d->dev, INPUT_REL_Y, (int8_t)y, true, K_FOREVER);
        }
    }
    if (gpio_pin_get_dt(&c->motion) > 0) {
        schedule(d, K_MSEC(c->interval));
        return;
    }
    gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_EDGE_TO_ACTIVE);
    /* Close the race between the last level read and enabling the interrupt. */
    if (gpio_pin_get_dt(&c->motion) > 0) {
        gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_DISABLE);
        schedule(d, K_NO_WAIT);
    }
}
static int paw_init(const struct device *dev) {
    const struct paw_config *c = dev->config;
    struct paw_data *d = dev->data;
    const struct gpio_dt_spec *pins[] = {&c->sck, &c->sdio, &c->motion, &c->auxiliary};
    for (int i = 0; i < ARRAY_SIZE(pins); i++) {
        if (!gpio_is_ready_dt(pins[i])) { return -ENODEV; }
    }
    int rc = gpio_pin_configure_dt(&c->sck, GPIO_OUTPUT_ACTIVE);
    if (rc < 0) { return rc; }
    rc = gpio_pin_configure_dt(&c->sdio, GPIO_INPUT);
    if (rc < 0) { return rc; }
    /* Matches the CoreX RMK wiring: auxiliary/NCS stays physically low. */
    rc = gpio_pin_configure_dt(&c->auxiliary, GPIO_OUTPUT_INACTIVE);
    if (rc < 0) { return rc; }
    rc = gpio_pin_configure_dt(&c->motion, GPIO_INPUT);
    if (rc < 0) { return rc; }
    d->dev = dev;
    gpio_init_callback(&d->irq, irq_handler, BIT(c->motion.pin));
    rc = gpio_add_callback(c->motion.port, &d->irq);
    if (rc < 0) { return rc; }
    k_work_init_delayable(&d->work, sample_work);
    k_work_queue_start(&d->queue, paw_stack, K_THREAD_STACK_SIZEOF(paw_stack),
                       K_PRIO_PREEMPT(10), NULL);
    schedule(d, K_MSEC(500));
    return 0;
}
#define PAW_DEFINE(n) \
    BUILD_ASSERT(DT_INST_PROP(n, res_cpi) >= 608 && DT_INST_PROP(n, res_cpi) <= 4826 \
                 && DT_INST_PROP(n, res_cpi) % 38 == 0, "Invalid PAW3222 CPI"); \
    BUILD_ASSERT(DT_INST_PROP(n, poll_interval_ms) > 0, "Invalid polling interval"); \
    static struct paw_data paw_data_##n; \
    static const struct paw_config paw_config_##n = { \
        .sck = GPIO_DT_SPEC_INST_GET(n, sck_gpios), \
        .sdio = GPIO_DT_SPEC_INST_GET(n, sdio_gpios), \
        .motion = GPIO_DT_SPEC_INST_GET(n, motion_gpios), \
        .auxiliary = GPIO_DT_SPEC_INST_GET(n, auxiliary_gpios), \
        .cpi = DT_INST_PROP(n, res_cpi), .interval = DT_INST_PROP(n, poll_interval_ms), \
    }; \
    DEVICE_DT_INST_DEFINE(n, paw_init, NULL, &paw_data_##n, &paw_config_##n, \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Select exactly one CoreX port");
DT_INST_FOREACH_STATUS_OKAY(PAW_DEFINE)
