/* SPDX-License-Identifier: MIT
 * Compile the actual transport with GPIO mocks. No Zephyr SDK required.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define BIT(n) (1u << (n))
#define GPIO_OUTPUT 1
#define GPIO_INPUT 2
struct gpio_dt_spec { int pin; };
struct paw_config { struct gpio_dt_spec sck, sdio, motion, auxiliary; };
static const struct paw_config cfg = {.sck = {1}, .sdio = {2}};
static int direction, data_level, clock_level, sent_count, samples;
static uint8_t sent[16], response;
static int gpio_pin_configure_dt(const struct gpio_dt_spec *p, int flags) {
    assert(p->pin == 2); direction = flags; return 0;
}
static int gpio_pin_set_dt(const struct gpio_dt_spec *p, int value) {
    if (p->pin == 2) { assert(direction == GPIO_OUTPUT); data_level = value; }
    else {
        assert(p->pin == 1);
        if (value && !clock_level && direction == GPIO_OUTPUT) {
            sent[sent_count / 8] = (sent[sent_count / 8] << 1) | data_level;
            sent_count++;
        }
        clock_level = value;
    }
    return 0;
}
static int gpio_pin_get_dt(const struct gpio_dt_spec *p) {
    assert(p->pin == 2 && direction == GPIO_INPUT);
    assert(clock_level == 1 && samples < 8);
    return !!(response & BIT(7 - samples++));
}
static void k_busy_wait(int us) { assert(us == 1 || us == 4); }
#include "../../drivers/corex/paw3222_wire.h"
static void reset(uint8_t value) {
    direction = GPIO_INPUT; clock_level = 1; sent_count = samples = 0;
    for (int i = 0; i < 16; i++) { sent[i] = 0; }
    response = value;
}
int main(void) {
    for (int value = 0; value < 256; value++) {
        reset(value);
        assert(read_reg(&cfg, 0x82) == value);
        assert(sent_count == 8 && sent[0] == 0x02 && samples == 8);
        assert(direction == GPIO_INPUT && clock_level == 1);
        reset(0);
        write_reg(&cfg, 0x09, value);
        assert(sent_count == 16 && sent[0] == 0x89 && sent[1] == value);
        assert(samples == 0 && direction == GPIO_INPUT && clock_level == 1);
    }
    puts("PAW3222: all 256 read/write values, address bits, SDIO turnaround and idle clock passed");
}
