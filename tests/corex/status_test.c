#include <assert.h>
#include <stdio.h>
#include "../../drivers/status/patterns.h"
static void dark(struct status_rgb c) { assert(!c.r && !c.g && !c.b); }
static int same(struct status_rgb a, struct status_rgb b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}
int main(void) {
    struct status_state s = STATUS_STATE_INIT;
    status_peer(&s, false, 0);
    assert(status_render(&s, 0).inner.b == 16);
    dark(status_render(&s, 400).inner);
    dark(status_render(&s, 10000).inner);
    status_peer(&s, true, 11000);
    assert(status_render(&s, 13999).inner.b == 16);
    dark(status_render(&s, 14000).inner);
    status_power(&s, true, 15000);
    assert(status_render(&s, 16000).inner.g == 32);
    status_battery(&s, 20, 16000);
    assert(status_render(&s, 16000).inner.r == 16);
    dark(status_render(&s, 16400).inner);
    assert(status_render(&s, 16800).inner.r == 16);
    assert(status_render(&s, 17200).inner.g > 0);
    status_battery(&s, 95, 18000);
    assert(status_render(&s, 18000).inner.g == 16);
    dark(status_render(&s, 21000).inner);
    status_battery(&s, 94, 22000);
    assert(status_render(&s, 23000).inner.g == 32);
    status_power(&s, false, 24000);
    dark(status_render(&s, 24000).inner);
    for (int p = 0; p < 5; p++) {
        int64_t t = 30000 + p * 20000;
        assert(status_profile(&s, p, false, true, t));
        struct status_rgb c = status_render(&s, t).outer;
        assert(same(c, status_profile_color(p)));
        for (int q = 0; q < p; q++) { assert(!same(c, status_profile_color(q))); }
        dark(status_render(&s, t+400).outer);
        assert(!status_profile(&s, p, false, true, t+500));
        assert(s.profile_since == t);
        dark(status_render(&s, t+10000).outer);
        status_profile(&s, p, true, false, t+11000);
        dark(status_render(&s, t+11600).outer);
    }
    /* Right half: outer = layer colour (always on), inner = own battery only. */
    struct status_state r = STATUS_STATE_INIT;
    r.layer_mode = true;
    status_peer(&r, false, 0);
    dark(status_render(&r, 0).inner);
    status_profile(&r, 1, false, true, 0);
    for (int l = 0; l < 9; l++) {
        r.layer = l;
        struct status_rgb c = status_render(&r, 100000).outer;
        assert(c.r || c.g || c.b);
        assert(same(c, status_layer_color(l)));
    }
    assert(!same(status_layer_color(0), status_layer_color(1)));
    status_battery(&r, 15, 200000);
    assert(status_render(&r, 200000).inner.r == 16);
    status_power(&r, true, 210000);
    assert(status_render(&r, 211000).inner.g == 32);
    puts("LED patterns: peer timing, priority, battery, charging, five profile colors, sync dedup and right layer colors passed");
}
