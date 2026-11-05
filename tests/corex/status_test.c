#include <assert.h>
#include <stdio.h>
#include "../../drivers/status/patterns.h"
static void dark(struct status_rgb c) { assert(!c.r && !c.g && !c.b); }
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
    for (int p = 0; p < 3; p++) {
        int64_t t = 30000 + p * 20000;
        assert(status_profile(&s, p, false, true, t));
        struct status_rgb c = status_render(&s, t).outer;
        assert(p == 0 ? c.g == 16 : p == 1 ? c.r == 16 : c.b == 16);
        dark(status_render(&s, t+400).outer);
        assert(!status_profile(&s, p, false, true, t+500));
        assert(s.profile_since == t);
        dark(status_render(&s, t+10000).outer);
        status_profile(&s, p, true, false, t+11000);
        dark(status_render(&s, t+11600).outer);
    }
    puts("LED patterns: peer timing, priority, battery, charging, profile colors and sync dedup passed");
}
