/* SPDX-License-Identifier: Apache-2.0
 * CoreX three-wire transport follows CoreX-RMK paw_wire.rs / paw3222.rs.
 * Register protocol: Copyright 2024 Google LLC, modifications 2025 sekigon-gonnoc,
 * Rust adaptation 2026 CoreX prototype. ZMK GPIO implementation 2026.
 * SDIO MUST be released while the sensor responds; SPIM with MOSI and MISO
 * on the same pin does not provide that guarantee.
 */
#define DT_DRV_COMPAT cornix_paw3222_gpio
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>
LOG_MODULE_REGISTER(cornix_paw, CONFIG_ZMK_LOG_LEVEL);

/* One connector (J4 or J3). The name is shared with the host wire test. */
struct paw_config {
    struct gpio_dt_spec sck, sdio, motion, auxiliary;
};
struct paw_dev_config {
    const struct paw_config *ports; /* devicetree order: J4, then J3 */
    uint8_t num_ports;
    uint16_t cpi, interval;
};
struct paw_data {
    const struct device *dev;
    struct gpio_callback irq;
    struct k_work_delayable work;
    struct k_work_q queue;
    const struct paw_config *port; /* connector with a responding sensor */
    atomic_t suspended;
    uint8_t misses;
};
/* Connectors use idle-high SCK and either framed NCS or legacy retained-low CS;
 * input comes only from the first one whose product ID checks out. */
K_THREAD_STACK_DEFINE(paw_stack, 1536);

#include "paw3222_wire.h"

static bool configure(const struct paw_config *c, uint16_t cpi) {
    if (read_reg(c, 0x00) != 0x30) { return false; }
    int cfg = read_reg(c, 0x06);
    if (cfg < 0 || write_reg(c, 0x06, cfg | 0x80) < 0) { return false; }
    k_sleep(K_MSEC(2));
    if (write_reg(c, 0x09, 0x5a) < 0) { return false; }
    int op = read_reg(c, 0x05);
    bool ok = op >= 0 && write_reg(c, 0x05, op | 0x18) >= 0 &&
              write_reg(c, 0x0d, cpi / 38) >= 0 && write_reg(c, 0x0e, cpi / 38) >= 0;
    int lock = write_reg(c, 0x09, 0);
    if (!ok || lock < 0) { return false; }
    if (read_reg(c, 0x02) < 0 || read_reg(c, 0x03) < 0 ||
        read_reg(c, 0x04) < 0 || read_reg(c, 0x12) < 0) { return false; }
    return read_reg(c, 0x00) == 0x30;
}
static void schedule(struct paw_data *d, k_timeout_t delay) {
    if (!atomic_get(&d->suspended)) {
        k_work_reschedule_for_queue(&d->queue, &d->work, delay);
    }
}
static void irq_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    ARG_UNUSED(port); ARG_UNUSED(pins);
    struct paw_data *d = CONTAINER_OF(cb, struct paw_data, irq);
    /* Never disarm the level-sensitive wake signal while entering System OFF. */
    if (atomic_get(&d->suspended)) { return; }
    const struct paw_config *c = d->port;
    if (c) { gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_DISABLE); }
    schedule(d, K_NO_WAIT);
}
static bool attach(struct paw_data *d, const struct paw_config *c) {
    gpio_init_callback(&d->irq, irq_handler, BIT(c->motion.pin));
    if (gpio_add_callback(c->motion.port, &d->irq) < 0) { return false; }
    d->port = c;
    return true;
}
static void detach(struct paw_data *d) {
    const struct paw_config *c = d->port;
    gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_DISABLE);
    d->port = NULL;
    gpio_remove_callback(c->motion.port, &d->irq);
}
static void sample_work(struct k_work *work) {
    struct paw_data *d = CONTAINER_OF(k_work_delayable_from_work(work), struct paw_data, work);
    const struct paw_dev_config *cfg = d->dev->config;
    if (atomic_get(&d->suspended)) { return; }
    if (!d->port) {
        for (int i = 0; i < cfg->num_ports; i++) {
            if (configure(&cfg->ports[i], cfg->cpi) && attach(d, &cfg->ports[i])) {
                LOG_INF("PAW3222 ready on port %u (%u CPI)", i, cfg->cpi);
                break;
            }
        }
        if (!d->port) {
            if (++d->misses >= CONFIG_CORNIX_PAW3222_PROBE_ATTEMPTS) {
                LOG_WRN("PAW3222 not found after %u probes; polling stopped", d->misses);
                return;
            }
            schedule(d, K_SECONDS(2));
            return;
        }
    }
    d->misses = 0;
    const struct paw_config *c = d->port;
    int status = read_reg(c, 0x02);
    if (status < 0 || (status == 0xff && read_reg(c, 0x00) != 0x30)) {
        detach(d);
        schedule(d, K_SECONDS(2));
        return;
    }
    if (status & BIT(7)) {
        int x, y;
        if (read_xy(c, &x, &y) == 0 && (x || y)) {
            input_report_rel(d->dev, INPUT_REL_X, (int8_t)x, false, K_FOREVER);
            input_report_rel(d->dev, INPUT_REL_Y, (int8_t)y, true, K_FOREVER);
        }
    }
    if (gpio_pin_get_dt(&c->motion) > 0) {
        schedule(d, K_MSEC(cfg->interval));
        return;
    }
    gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_EDGE_TO_ACTIVE);
    /* Close the race between the last level read and enabling the interrupt. */
    if (gpio_pin_get_dt(&c->motion) > 0) {
        gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_DISABLE);
        schedule(d, K_NO_WAIT);
    }
}
static int port_init(const struct paw_config *c) {
    const struct gpio_dt_spec *pins[] = {&c->sck, &c->sdio, &c->motion, &c->auxiliary};
    for (int i = 0; i < ARRAY_SIZE(pins); i++) {
        if (!gpio_is_ready_dt(pins[i])) { return -ENODEV; }
    }
    int rc = gpio_pin_configure_dt(&c->sck, GPIO_OUTPUT_ACTIVE);
    if (rc < 0) { return rc; }
    rc = gpio_pin_configure_dt(&c->sdio, GPIO_INPUT);
    if (rc < 0) { return rc; }
    /* ACTIVE_LOW: deselected high. Legacy ACTIVE_HIGH: retained low. */
    rc = gpio_pin_configure_dt(&c->auxiliary, GPIO_OUTPUT_INACTIVE);
    if (rc < 0) { return rc; }
    return gpio_pin_configure_dt(&c->motion, GPIO_INPUT);
}
static int paw_init(const struct device *dev) {
    const struct paw_dev_config *cfg = dev->config;
    struct paw_data *d = dev->data;
    for (int i = 0; i < cfg->num_ports; i++) {
        int rc = port_init(&cfg->ports[i]);
        if (rc < 0) { return rc; }
    }
    d->dev = dev;
    k_work_init_delayable(&d->work, sample_work);
    k_work_queue_start(&d->queue, paw_stack, K_THREAD_STACK_SIZEOF(paw_stack),
                       K_PRIO_PREEMPT(10), NULL);
    schedule(d, K_MSEC(500));
    return 0;
}
#if IS_ENABLED(CONFIG_PM_DEVICE)
static void resume_sampling(struct paw_data *d) {
    if (d->port) { gpio_pin_interrupt_configure_dt(&d->port->motion, GPIO_INT_DISABLE); }
    atomic_clear(&d->suspended);
    schedule(d, K_NO_WAIT);
}
static int paw_pm_action(const struct device *dev, enum pm_device_action action) {
    struct paw_data *d = dev->data;
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND: {
        atomic_set(&d->suspended, 1);
        struct k_work_sync sync;
        k_work_cancel_delayable_sync(&d->work, &sync);
        /* The worker may have changed ports before cancellation completed. */
        const struct paw_config *c = d->port;
        if (!c) {
            /* Never power off a keyless central without an available wake pin.
             * Do not resurrect exhausted sensor probes on each sleep attempt. */
            atomic_clear(&d->suspended);
            if (d->misses < CONFIG_CORNIX_PAW3222_PROBE_ATTEMPTS) {
                schedule(d, K_SECONDS(2)); /* Continue the cancelled probe. */
            }
            return -EBUSY;
        }
        int level = gpio_pin_get_dt(&c->motion);
        if (level != 0) {
            resume_sampling(d);
            return level < 0 ? level : -EBUSY;
        }
        /* configure() already preserves the sensor's low-power rest modes.
         * Level sensing is retained by nRF52840 System OFF, unlike an edge.
         */
        int rc = gpio_pin_interrupt_configure_dt(&c->motion, GPIO_INT_LEVEL_ACTIVE);
        if (rc < 0) { resume_sampling(d); return rc; }
        level = gpio_pin_get_dt(&c->motion);
        if (level != 0) {
            resume_sampling(d);
            return level < 0 ? level : -EBUSY;
        }
        return 0;
    }
    case PM_DEVICE_ACTION_RESUME:
        if (atomic_get(&d->suspended)) {
            d->misses = 0;
            resume_sampling(d);
        }
        return 0;
    default:
        return -ENOTSUP;
    }
}
#endif
#define PAW_PORT(node_id, prop, idx) { \
        .sck = GPIO_DT_SPEC_GET_BY_IDX(node_id, sck_gpios, idx), \
        .sdio = GPIO_DT_SPEC_GET_BY_IDX(node_id, sdio_gpios, idx), \
        .motion = GPIO_DT_SPEC_GET_BY_IDX(node_id, motion_gpios, idx), \
        .auxiliary = GPIO_DT_SPEC_GET_BY_IDX(node_id, auxiliary_gpios, idx), \
    }
#define PAW_DEFINE(n) \
    BUILD_ASSERT(DT_INST_PROP(n, res_cpi) >= 608 && DT_INST_PROP(n, res_cpi) <= 4826 \
                 && DT_INST_PROP(n, res_cpi) % 38 == 0, "Invalid PAW3222 CPI"); \
    BUILD_ASSERT(DT_INST_PROP(n, poll_interval_ms) > 0, "Invalid polling interval"); \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, sdio_gpios) == DT_INST_PROP_LEN(n, sck_gpios) && \
                 DT_INST_PROP_LEN(n, motion_gpios) == DT_INST_PROP_LEN(n, sck_gpios) && \
                 DT_INST_PROP_LEN(n, auxiliary_gpios) == DT_INST_PROP_LEN(n, sck_gpios), \
                 "Each connector needs SCK, SDIO, MOTION and AUX"); \
    static struct paw_data paw_data_##n; \
    static const struct paw_config paw_ports_##n[] = { \
        DT_INST_FOREACH_PROP_ELEM_SEP(n, sck_gpios, PAW_PORT, (,)) \
    }; \
    static const struct paw_dev_config paw_config_##n = { \
        .ports = paw_ports_##n, .num_ports = ARRAY_SIZE(paw_ports_##n), \
        .cpi = DT_INST_PROP(n, res_cpi), .interval = DT_INST_PROP(n, poll_interval_ms), \
    }; \
    PM_DEVICE_DT_INST_DEFINE(n, paw_pm_action); \
    DEVICE_DT_INST_DEFINE(n, paw_init, PM_DEVICE_DT_INST_GET(n), &paw_data_##n, &paw_config_##n, \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Enable exactly one Cornix sensor node");
DT_INST_FOREACH_STATUS_OKAY(PAW_DEFINE)
