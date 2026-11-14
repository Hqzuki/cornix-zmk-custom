#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../drivers/status/patterns.h"
static void dark(struct status_rgb c) { assert(!c.r && !c.g && !c.b); }
static int same(struct status_rgb a, struct status_rgb b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}
static void bluetooth_battery(void) {
    const uint8_t levels[] = {0, 20, 21, 50, 51, 100};
    const struct status_rgb colors[] = {
        {13,0,0}, {13,0,0}, {13,8,0}, {13,8,0}, {0,13,0}, {0,13,0},
    };
    for (int central = 0; central <= 1; central++) {
        struct status_state s = STATUS_STATE_INIT_ROLE(central);
        status_layer(&s, true, 7);
        status_profile(&s, 0, true, false, 0);
        status_peer(&s, true, 0);
        status_power(&s, true, 0);
        dark(status_render(&s, 1000).inner); /* Unknown, even during charging. */
        status_battery(&s, 101, 0);
        assert(!s.battery_valid);
        for (unsigned i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
            status_battery(&s, levels[i], 0);
            assert(s.battery_valid);
            for (int t = 0; t <= 4000; t += 100) {
                struct status_frame f = status_render(&s, t);
                assert(same(f.inner, colors[i])); /* No warning/breathing gaps. */
                assert(same(f.outer, central ? status_profile_color(0) :
                                              status_layer_color(7)));
            }
        }
        status_battery(&s, 101, 0);
        assert(s.battery == 100); /* Invalid samples cannot overwrite a valid one. */
        status_battery(&s, 50, 0);
        status_layer(&s, true, 1);
        assert(same(status_render(&s, 5000).inner, (struct status_rgb){0,13,0}));
        status_layer(&s, true, 7);
        assert(same(status_render(&s, 5000).inner, (struct status_rgb){13,8,0}));
        s.idle = true;
        assert(same(status_render(&s, 5000).inner, (struct status_rgb){0,13,0}));
        dark(status_render(&s, 5000).outer);
        s.idle = false;
        status_power(&s, false, 5000);
        assert(same(status_render(&s, 5000).inner, (struct status_rgb){13,8,0}));
        status_layer(&s, true, 1);
        dark(status_render(&s, 5000).inner);
        /* Exit to normal peer/low-battery priorities, without restarting them. */
        status_peer(&s, false, 6000);
        status_layer(&s, true, 7);
        assert(same(status_render(&s, 6000).inner, (struct status_rgb){13,8,0}));
        status_layer(&s, true, 1);
        if (central) { dark(status_render(&s, 6000).inner); }
        else { assert(status_render(&s, 6000).inner.b == 13); }
        status_battery(&s, 0, 7000);
        status_layer(&s, true, 7);
        assert(status_render(&s, 7500).inner.r == 13);
        status_layer(&s, true, 1);
        dark(status_render(&s, 7500).inner); /* Normal low warning's dark gap. */
    }
    /* Sync the layer, but never copy the central's battery to the left. */
    struct status_state r = STATUS_STATE_INIT_ROLE(true);
    struct status_state l = STATUS_STATE_INIT_ROLE(false);
    status_battery(&r, 80, 0);
    status_battery(&l, 10, 0);
    status_layer(&r, true, 7);
    assert(status_receive(&l, 0, status_sync_flags(&r), 0));
    assert(status_render(&r, 500).inner.g == 13);
    assert(status_render(&l, 500).inner.r == 13);
    assert(status_receive(&l, 0, 0, 1000)); /* Lost layer state: no stale preview. */
    dark(status_render(&l, 1500).inner);
}
int main(void) {
    bluetooth_battery();
    struct status_state s = STATUS_STATE_INIT;
    status_peer(&s, false, 0);
    assert(status_render(&s, 0).inner.b == 13);
    dark(status_render(&s, 400).inner);
    dark(status_render(&s, 10000).inner);
    status_peer(&s, true, 11000);
    assert(status_render(&s, 13999).inner.b == 13);
    dark(status_render(&s, 14000).inner);
    status_power(&s, true, 15000);
    assert(status_render(&s, 16000).inner.g == 13);
    status_battery(&s, 20, 16000);
    assert(status_render(&s, 16000).inner.r == 13);
    dark(status_render(&s, 16400).inner);
    assert(status_render(&s, 16800).inner.r == 13);
    assert(status_render(&s, 17200).inner.r == 0); /* low warning ended */
    assert(status_render(&s, 17300).inner.g > 0); /* breathing resumed */
    status_battery(&s, 95, 18000);
    assert(status_render(&s, 18000).inner.g == 13);
    assert(status_render(&s, 21000).inner.g == 13); /* steady, no 3 s cutoff */
    assert(status_render(&s, 900000).inner.g == 13);
    status_battery(&s, 94, 21500);
    assert(status_render(&s, 21500).inner.g == 13); /* hysteresis keeps full */
    status_battery(&s, 89, 22000);
    assert(status_render(&s, 23000).inner.g == 13); /* back to breathing */
    status_power(&s, false, 24000);
    dark(status_render(&s, 24000).inner);
    for (int p = 0; p < 5; p++) {
        int64_t t = 30000 + p * 20000;
        assert(status_profile(&s, p, false, true, t));
        struct status_rgb c = status_render(&s, t).outer;
        assert(same(c, status_profile_color(p)));
        for (int q = 0; q < p; q++) { assert(!same(c, status_profile_color(q))); }
        assert(same(status_render(&s, t+400).outer, c));
        assert(!status_profile(&s, p, false, true, t+500));
        assert(s.profile_since == t);
        assert(same(status_render(&s, t+10000).outer, c));
        status_profile(&s, p, true, false, t+11000);
        assert(same(status_render(&s, t+11600).outer, c));
        s.idle = true;
        dark(status_render(&s, t+1000000).outer);
        s.idle = false;
        assert(same(status_render(&s, t+1000001).outer, c)); /* wake: no 600 ms cutoff */
        status_profile(&s, p, false, false, t+12000);
        assert(same(status_render(&s, t+13000).outer, c));
    }
    /* Right (central): outer = host profile, inner = own battery only. */
    struct status_state r = STATUS_STATE_INIT_ROLE(true);
    status_peer(&r, false, 0);
    dark(status_render(&r, 0).inner);
    status_layer(&r, true, 6);
    assert(status_profile(&r, 3, false, true, 1000));
    assert(same(status_render(&r, 1000).outer, status_profile_color(3)));
    status_battery(&r, 15, 200000);
    assert(status_render(&r, 200000).inner.r == 13);
    status_power(&r, true, 210000);
    assert(status_render(&r, 211000).inner.g == 13);

    /* Left (peripheral): outer = synced layer colour, always on; inner unchanged. */
    struct status_state l = STATUS_STATE_INIT_ROLE(false);
    status_peer(&l, true, 0);
    assert(status_render(&l, 0).inner.b == 13);
    dark(status_render(&l, 0).outer);
    for (int k = 0; k < 9; k++) {
        r.layer = k;
        assert(status_receive(&l, r.profile, status_sync_flags(&r), 300000 + k));
        struct status_rgb c = status_render(&l, 400000).outer;
        assert(k == 6 ? !(c.r || c.g || c.b) : (c.r || c.g || c.b)); /* MOUSE dark */
        assert(same(c, status_layer_color(k)));
    }
    assert(!same(status_layer_color(0), status_layer_color(1)));
    for (int a = 0; a < 9; a++) {
        for (int b = a + 1; b < 9; b++) {
            assert(!same(status_layer_color(a), status_layer_color(b)));
        }
    }
    assert(!status_receive(&l, 0, 1u << 16, 0));
    assert(!status_receive(&l, 256, 0, 0));
    assert(status_receive(&l, 0, 1, 500000));
    dark(status_render(&l, 500000).outer);
    /* Idle: both outer LEDs go dark; battery/charging stays inside;
     * the left wakes on right activity. */
    status_battery(&r, 15, 600000);
    r.idle = true;
    assert(status_render(&r, 600000).inner.r == 13);
    dark(status_render(&r, 600000).outer);
    status_battery(&r, 50, 601200);
    assert(status_render(&r, 603000).inner.g > 0); /* still charging (powered) */
    status_peer(&l, false, 600000);
    l.idle = true;
    dark(status_render(&l, 600000).inner); /* no peer blink while idle */
    r.layer = 2;
    assert(!(status_sync_flags(&r) & STATUS_SYNC_ACTIVE));
    assert(status_receive(&l, r.profile, status_sync_flags(&r), 600000));
    assert(!l.remote_active);
    l.idle = true;
    dark(status_render(&l, 600000).outer);
    r.idle = false;
    assert(same(status_render(&r, 601000).outer, status_profile_color(3)));
    assert(status_receive(&l, r.profile, status_sync_flags(&r), 601000));
    assert(l.remote_active);
    l.idle = false;
    assert(same(status_render(&l, 601000).outer, status_layer_color(2)));
    puts("LED patterns: Bluetooth battery preview, battery/charging, active profile colors, idle blackout and wake restoration on both halves passed");
}
