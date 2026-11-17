/* SPDX-License-Identifier: Apache-2.0
 * PAW3222 register protocol based on the Google LLC / sekigon-gonnoc work.
 * Cornix ZMK GPIO integration.
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
#include <string.h>
LOG_MODULE_REGISTER(cornix_paw, CONFIG_ZMK_LOG_LEVEL);

/* Candidate GPIOs and the detected signal-role mapping, kept in RAM only. */
struct paw_config {
    struct gpio_dt_spec candidates[4];
    struct gpio_dt_spec sck, sdio, motion, auxiliary;
    bool probing;
};
struct paw_dev_config {
    const struct paw_config *pins;
    uint16_t cpi, interval;
};
struct paw_data {
    const struct device *dev;
    struct gpio_callback irq;
    struct k_work_delayable work;
    struct k_work_q queue;
    const struct paw_config *port; /* connector with a responding sensor */
    struct paw_config active_port;
    atomic_t suspended;
    uint8_t misses;
};
/* Probe every signal-pin permutation on boot; no mapping is written to flash. */
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
static void release_candidates(const struct paw_config *base) {
    for (int i = 0; i < ARRAY_SIZE(base->candidates); i++) {
        gpio_pin_configure_dt(&base->candidates[i], GPIO_INPUT);
    }
}
static void make_role(struct gpio_dt_spec *dst, const struct gpio_dt_spec *src,
                      bool active_low, bool pull_up) {
    *dst = *src;
    dst->dt_flags &= ~(GPIO_ACTIVE_LOW | GPIO_PULL_UP);
    if (active_low) { dst->dt_flags |= GPIO_ACTIVE_LOW; }
    if (pull_up) { dst->dt_flags |= GPIO_PULL_UP; }
}
static bool identify_permutation(struct paw_config *candidate) {
    /* Open-drain outputs reduce contention if a candidate is the MOTION output. */
    if (gpio_pin_configure_dt(&candidate->sck,
                              GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_PULL_UP) < 0 ||
        gpio_pin_set_dt(&candidate->sck, 1) < 0 ||
        gpio_pin_configure_dt(&candidate->sdio, GPIO_INPUT | GPIO_PULL_UP) < 0 ||
        gpio_pin_configure_dt(&candidate->auxiliary,
                              GPIO_OUTPUT | GPIO_OPEN_DRAIN | GPIO_PULL_UP) < 0 ||
        gpio_pin_set_dt(&candidate->auxiliary, 0) < 0 ||
        gpio_pin_configure_dt(&candidate->motion, GPIO_INPUT | GPIO_PULL_UP) < 0) {
        return false;
    }
    int id1a = read_reg(candidate, 0x00);
    int id1b = read_reg(candidate, 0x00);
    int id2a = read_reg(candidate, 0x01);
    int id2b = read_reg(candidate, 0x01);
    if (id1a != 0x30 || id1a != id1b || id2a < 0 || id2a == 0x00 ||
        id2a == 0xff || id2a != id2b) {
        return false;
    }
    LOG_INF("PAW3222 candidate product IDs: 0x%02x / 0x%02x", id1a, id2a);
    return true;
}
static bool activate_detected_pins(struct paw_config *c) {
    /* Set inactive/high output values atomically when leaving open-drain probe mode. */
    c->probing = false;
    return gpio_pin_configure_dt(&c->sck, GPIO_OUTPUT_ACTIVE) == 0 &&
           gpio_pin_configure_dt(&c->sdio, GPIO_INPUT | GPIO_PULL_UP) == 0 &&
           gpio_pin_configure_dt(&c->auxiliary, GPIO_OUTPUT_INACTIVE) == 0 &&
           gpio_pin_configure_dt(&c->motion, GPIO_INPUT | GPIO_PULL_UP) == 0;
}
static bool discover(struct paw_data *d, const struct paw_config *base, uint16_t cpi) {
    /* Prefer the standard LisM map (D10/D9/D8/D7), then try every other map. */
    static const int probe_order[] = {3, 2, 1, 0};
    static const uint8_t dnum[] = {7, 8, 9, 10};
    for (int si = 0; si < 4; si++) {
        int sck = probe_order[si];
        for (int di = 0; di < 4; di++) {
            int sdio = probe_order[di];
            if (sdio == sck) { continue; }
            for (int ci = 0; ci < 4; ci++) {
                int ncs = probe_order[ci];
                if (ncs == sck || ncs == sdio) { continue; }
                int motion = 6 - sck - sdio - ncs;
                struct paw_config candidate = {0};
                memcpy(candidate.candidates, base->candidates, sizeof(candidate.candidates));
                candidate.probing = true;
                make_role(&candidate.sck, &base->candidates[sck], false, true);
                make_role(&candidate.sdio, &base->candidates[sdio], false, true);
                make_role(&candidate.auxiliary, &base->candidates[ncs], true, true);
                make_role(&candidate.motion, &base->candidates[motion], true, true);
                if (identify_permutation(&candidate) && activate_detected_pins(&candidate) &&
                    configure(&candidate, cpi)) {
                    d->active_port = candidate;
                    d->port = &d->active_port;
                    LOG_INF("PAW3222 map: SCK D%u, SDIO D%u, CS D%u, MOTION D%u",
                            dnum[sck], dnum[sdio], dnum[ncs], dnum[motion]);
                    return true;
                }
                release_candidates(base);
                k_busy_wait(50);
            }
        }
    }
    release_candidates(base);
    return false;
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
        const struct paw_config *base = cfg->pins;
        if (!discover(d, base, cfg->cpi) || !attach(d, &d->active_port)) {
            d->port = NULL;
            if (++d->misses >= CONFIG_CORNIX_PAW3222_PROBE_ATTEMPTS) {
                LOG_WRN("PAW3222 signal map not found after %u scans; polling stopped", d->misses);
                return;
            }
            schedule(d, K_SECONDS(2));
            return;
        }
        LOG_INF("PAW3222 ready (%u CPI)", cfg->cpi);
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
    const struct gpio_dt_spec *pins[] = {&c->candidates[0], &c->candidates[1],
                                         &c->candidates[2], &c->candidates[3]};
    for (int i = 0; i < ARRAY_SIZE(pins); i++) {
        if (!gpio_is_ready_dt(pins[i])) { return -ENODEV; }
        int rc = gpio_pin_configure_dt(pins[i], GPIO_INPUT);
        if (rc < 0) { return rc; }
    }
    return 0;
}
static int paw_init(const struct device *dev) {
    const struct paw_dev_config *cfg = dev->config;
    struct paw_data *d = dev->data;
    int rc = port_init(cfg->pins);
    if (rc < 0) { return rc; }
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
#define PAW_DEFINE(n) \
    BUILD_ASSERT(DT_INST_PROP(n, res_cpi) >= 608 && DT_INST_PROP(n, res_cpi) <= 4826 \
                 && DT_INST_PROP(n, res_cpi) % 38 == 0, "Invalid PAW3222 CPI"); \
    BUILD_ASSERT(DT_INST_PROP(n, poll_interval_ms) > 0, "Invalid polling interval"); \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, autodetect_gpios) == 4, \
                 "Autodetection requires exactly four signal GPIOs"); \
    static struct paw_data paw_data_##n; \
    static const struct paw_config paw_pins_##n = { \
        .candidates = { \
            GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), autodetect_gpios, 0), \
            GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), autodetect_gpios, 1), \
            GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), autodetect_gpios, 2), \
            GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), autodetect_gpios, 3), \
        }, \
    }; \
    static const struct paw_dev_config paw_config_##n = { \
        .pins = &paw_pins_##n, \
        .cpi = DT_INST_PROP(n, res_cpi), .interval = DT_INST_PROP(n, poll_interval_ms), \
    }; \
    PM_DEVICE_DT_INST_DEFINE(n, paw_pm_action); \
    DEVICE_DT_INST_DEFINE(n, paw_init, PM_DEVICE_DT_INST_GET(n), &paw_data_##n, &paw_config_##n, \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Enable exactly one Cornix sensor node");
DT_INST_FOREACH_STATUS_OKAY(PAW_DEFINE)
