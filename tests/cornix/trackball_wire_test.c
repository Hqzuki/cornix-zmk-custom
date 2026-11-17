/* SPDX-License-Identifier: MIT
 * Exercise the actual GPIO transport, including failed transactions.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#define BIT(n) (1u << (n))
#define GPIO_ACTIVE_LOW 1
#define GPIO_OUTPUT 1
#define GPIO_INPUT 2
#define GPIO_OPEN_DRAIN 4
#define GPIO_PULL_UP 8
struct gpio_dt_spec { int pin, dt_flags; };
struct paw_config { struct gpio_dt_spec sck, sdio, motion, auxiliary; };
static const struct paw_config cfg = {.sck = {1,0}, .sdio = {2,0}, .auxiliary = {3,GPIO_ACTIVE_LOW}};
static int direction, data_level, clock_level, sent_count, samples, cs_level, cs_pulses;
static int fail_sample, fail_set, fail_direction;
static bool framed;
static uint8_t sent[16], response[2];
static int gpio_pin_configure_dt(const struct gpio_dt_spec *p, int flags) {
    assert(p->pin == 2);
    if (fail_direction && (flags & fail_direction)) { fail_direction=0; return -EIO; }
    direction = flags; return 0;
}
static int gpio_pin_set_dt(const struct gpio_dt_spec *p, int value) {
    if (fail_set == p->pin) { fail_set=0; return -EIO; }
    if (p->pin == 3) {
        if (value && !cs_level) { cs_pulses++; }
        cs_level = value;
    } else if (p->pin == 2) {
        assert((!framed || cs_level) && (direction & GPIO_OUTPUT)); data_level=value;
    } else {
        assert(p->pin==1);
        if (value && !clock_level && (direction & GPIO_OUTPUT)) {
            assert(!framed || cs_level);
            sent[sent_count/8]=(sent[sent_count/8]<<1)|data_level; sent_count++;
        }
        clock_level=value;
    }
    return 0;
}
static int gpio_pin_get_dt(const struct gpio_dt_spec *p) {
    assert(p->pin==2 && (direction & GPIO_INPUT));
    assert((!framed || cs_level) && clock_level==1 && samples<16);
    if (samples==fail_sample) { fail_sample=-1; return -EIO; }
    int bit=7-samples%8, byte=samples/8; samples++;
    return !!(response[byte]&BIT(bit));
}
static void k_busy_wait(int us) { assert(us==1 || us==4); }
#include "../../drivers/trackball/paw3222_wire.h"
static void reset(uint8_t value) {
    direction=GPIO_INPUT; clock_level=1; sent_count=samples=cs_level=cs_pulses=0;
    fail_sample=-1; fail_set=fail_direction=0; framed=true;
    for (int i=0; i<16; i++) { sent[i]=0; }
    response[0]=response[1]=value;
}
static void idle(void) { assert((direction & GPIO_INPUT) && clock_level==1 && cs_level==0); }
int main(void) {
    for (int value=0; value<256; value++) {
        reset(value);
        assert(read_reg(&cfg,0x82)==value);
        assert(sent_count==8 && sent[0]==0x02 && samples==8 && cs_pulses==1); idle();
        reset(0);
        assert(write_reg(&cfg,0x09,value)==0);
        assert(sent_count==16 && sent[0]==0x89 && sent[1]==value && cs_pulses==1); idle();
    }
    for (int x=0; x<256; x+=5) for (int y=0; y<256; y+=7) {
        reset(0); response[0]=x; response[1]=y;
        int rx=-1,ry=-1;
        assert(read_xy(&cfg,&rx,&ry)==0 && rx==x && ry==y);
        assert(sent_count==16 && sent[0]==3 && sent[1]==4 && cs_pulses==1); idle();
    }
    for (int failure=0; failure<16; failure++) {
        reset(0); fail_sample=failure;
        int x=1234,y=5678;
        assert(read_xy(&cfg,&x,&y)==-EIO && x==1234 && y==5678); idle();
    }
    for (int pin=1; pin<=3; pin++) {
        reset(0); fail_set=pin;
        assert(read_reg(&cfg,0)==-EIO); idle();
    }
    reset(0); fail_direction=GPIO_OUTPUT;
    assert(write_reg(&cfg,0,1)==-EIO); idle();
    reset(0); fail_direction=GPIO_INPUT;
    assert(read_reg(&cfg,0)==-EIO); idle();
    /* Legacy boards can retain their physically-low CS wiring. */
    struct paw_config legacy=cfg; legacy.auxiliary.dt_flags=0;
    reset(42); framed=false;
    assert(read_reg(&legacy,0)==42 && cs_pulses==0); idle();
    puts("PAW3222: 256 values, half-duplex XY framing, GPIO failure cleanup and legacy CS passed");
}
