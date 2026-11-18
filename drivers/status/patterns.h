/* SPDX-License-Identifier: MIT
 * Shared Cornix status patterns. Timing and breathing LUT follow the
 * Cornix indicator, Copyright (c) 2026 numachang.
 * Pure state/render logic shared by the LED controllers.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

struct status_rgb { uint8_t r, g, b; };
struct status_frame { struct status_rgb inner, outer; };
struct status_state {
    bool peer_known, peer_connected;
    int64_t peer_since;
    bool profile_valid, host_connected, host_open;
    uint8_t profile, battery;
    bool battery_valid;
    int64_t profile_since, low_since, full_since, charge_since;
    bool powered, full_shown;
    /* Roles: the Cornix right (central) shows the host profile on its outer
     * LED and only its own battery/charging inside (hide_peer). The stock
     * left shows the central's active layer on its outer LED (layer_outer). */
    bool hide_peer, layer_outer, layer_valid;
    uint8_t layer;
    /* ZMK activity state is still synchronized for split behavior and sleep.
     * It does not select a separate LED mode. */
    bool idle, remote_active;
};

/* Single source of defaults for firmware. ROLE: central = Cornix
 * right (host profile outside), peripheral = stock left (layer outside). */
#define STATUS_STATE_DEFAULTS .low_since = -1, .full_since = -1
#define STATUS_STATE_INIT {STATUS_STATE_DEFAULTS}
#define STATUS_STATE_INIT_ROLE(central) \
    {STATUS_STATE_DEFAULTS, .hide_peer = (central), .layer_outer = !(central)}

static inline void status_peer(struct status_state *s, bool connected, int64_t now) {
    if (!s->peer_known || s->peer_connected != connected) {
        s->peer_known = true;
        s->peer_connected = connected;
        s->peer_since = now;
    }
}

static inline bool status_profile(struct status_state *s, uint8_t index, bool connected,
                                  bool open, int64_t now) {
    if (s->profile_valid && s->profile == index && s->host_connected == connected &&
        s->host_open == open) {
        return false;
    }
    s->profile_valid = true;
    s->profile = index;
    s->host_connected = connected;
    s->host_open = open;
    s->profile_since = now;
    return true;
}

static inline void status_layer(struct status_state *s, bool valid, uint8_t layer) {
    s->layer_valid = valid;
    s->layer = layer;
}

/* Right-to-left sync payload: param2 bit0 host connected, bit1 host open,
 * bit2 layer present, bit3 right active, bits 8-15 layer id. */
#define STATUS_SYNC_LAYER  4u
#define STATUS_SYNC_ACTIVE 8u
#define STATUS_SYNC_MASK   0xff0fu

static inline uint32_t status_sync_flags(const struct status_state *s) {
    return (uint32_t)s->host_connected | ((uint32_t)s->host_open << 1) |
           (s->idle ? 0 : STATUS_SYNC_ACTIVE) |
           (s->layer_valid ? STATUS_SYNC_LAYER | ((uint32_t)s->layer << 8) : 0);
}

static inline bool status_receive(struct status_state *s, uint32_t profile, uint32_t flags,
                                  int64_t now) {
    if (profile > 255 || (flags & ~STATUS_SYNC_MASK)) {
        return false;
    }
    status_profile(s, profile, flags & 1, flags & 2, now);
    status_layer(s, flags & STATUS_SYNC_LAYER, (flags >> 8) & 0xff);
    s->remote_active = flags & STATUS_SYNC_ACTIVE;
    return true;
}

static inline void status_battery(struct status_state *s, uint8_t level, int64_t now) {
    if (level > 100) {
        return;
    }
    s->battery = level;
    s->battery_valid = true;

    if (level <= 20) {
        s->low_since = now;
    }

    if (s->powered && level >= 95 && !s->full_shown) {
        s->full_shown = true;
        s->full_since = now;
    } else if (s->powered && level < 90 && s->full_shown) {
        /* Hysteresis: once full, stay steady green until below 90%. */
        s->full_shown = false;
        s->full_since = -1;
        s->charge_since = now;
    }
}

static inline void status_power(struct status_state *s, bool powered, int64_t now) {
    if (s->powered == powered) {
        return;
    }
    s->powered = powered;
    s->charge_since = now;
    s->full_shown = powered && s->battery >= 95;
    s->full_since = s->full_shown ? now : -1;
}

/* Host BT profiles 0-4 (BLUETOOTH layer selects all five). */
static inline struct status_rgb status_profile_color(uint8_t profile) {
    static const struct status_rgb colors[] = {
        {0,0,13}, {13,0,0}, {0,13,0}, {13,8,0}, {10,0,13},
    };
    return profile < 5 ? colors[profile] : (struct status_rgb){7,7,7};
}

/* Layer ids follow the keymap definition order for each firmware variant. */
static inline struct status_rgb status_layer_color(uint8_t layer) {
#if defined(CONFIG_CORNIX_TB_CENTRAL) || defined(CONFIG_CORNIX_RIGHT_PERIPHERAL)
    /* TB: WIN, MAC, NUMFN, PAN, GESTURE, MOUSE, BLUETOOTH. */
    static const struct status_rgb colors[] = {
        {13,0,0},   /* 0 WIN: red */
        {0,0,13},   /* 1 MAC: blue */
        {0,13,0},   /* 2 NUMFN: green */
        {13,8,0},   /* 3 PAN: yellow */
        {13,0,13},  /* 4 GESTURE: magenta */
        {0,0,0},    /* 5 MOUSE: dark */
        {8,8,8},    /* 6 BLUETOOTH: white */
    };
    return layer < 7 ? colors[layer] : (struct status_rgb){0};
#else
    /* Stock: WIN, MAC, NUM, FN, PAN, GESTURE, MOUSE, BLUETOOTH, NAV. */
    static const struct status_rgb colors[] = {
        {13,0,0},   /* 0 WIN: red */
        {0,0,13},   /* 1 MAC: blue */
        {0,13,0},   /* 2 NUM: green */
        {13,8,0},   /* 3 FN: yellow */
        {13,0,13},  /* 4 PAN: magenta */
        {0,10,10},  /* 5 GESTURE: cyan */
        {0,0,0},    /* 6 MOUSE: dark */
        {8,8,8},    /* 7 BLUETOOTH: white */
        {13,3,0},   /* 8 NAV: orange */
    };
    return layer < 9 ? colors[layer] : (struct status_rgb){0};
#endif
}

static inline struct status_frame status_render(const struct status_state *s, int64_t now) {
    struct status_frame f = {0};

    const struct status_rgb red = {13,0,0};
    const struct status_rgb blue = {0,0,13};
    const struct status_rgb green = {0,13,0};

    int64_t low = now - s->low_since;
    int64_t peer = now - s->peer_since;

    /* Each half uses its own latest battery sample. Keep the outer layer/
     * profile indicator; temporarily replace only the inner LED while BT is
     * the displayed layer. Unknown is dark, distinct from a measured 0%. */
    const uint8_t bluetooth_layer =
#if defined(CONFIG_CORNIX_TB_CENTRAL) || defined(CONFIG_CORNIX_RIGHT_PERIPHERAL)
        6;
#else
        7;
#endif
    if (s->layer_valid && s->layer == bluetooth_layer) {
        if (s->battery_valid) {
            f.inner = s->battery <= 20 ? red :
                      s->battery <= 50 ? (struct status_rgb){13,8,0} : green;
        }

    /* Reserve the whole low-battery pattern, including its dark gaps. */
    } else if (s->low_since >= 0 && low >= 0 && low < 1200) {
        if (low < 400 || low >= 800) {
            f.inner = red;
        }

    } else if (!s->hide_peer && s->peer_known && peer >= 0 &&
               peer < (s->peer_connected ? 3000 : 10000)) {
        if (s->peer_connected || peer % 1000 < 400) {
            f.inner = blue;
        }

    } else if (s->powered) {
        if (s->full_shown) {
            f.inner = green; /* charge complete: steady while USB stays connected */
        } else {
            static const uint8_t breath[60] = {
                0,0,0,1,1,2,3,4,5,7,8,9,11,13,14,16,18,19,21,23,
                24,25,27,28,29,30,31,31,32,32,32,32,31,31,30,29,28,27,25,24,
                23,21,19,18,16,14,13,11,9,8,7,5,4,3,2,1,1,0,0,0
            };
            /* Scale the original 0..32 curve to a maximum of 13/255. */
            f.inner.g = breath[((now - s->charge_since) % 2000) * 60 / 2000] * 13 / 32;
        }
    }

    if (s->layer_outer) {
        if (s->layer_valid) {
            if (s->layer == 0 || s->layer == 1) {
                /* WIN: red / MAC: blue, breathing over a 3-second cycle. */
                static const uint8_t layer_breath[60] = {
                    0,0,0,1,1,2,3,4,5,7,8,9,11,13,14,16,18,19,21,23,
                    24,25,27,28,29,30,31,31,32,32,32,32,31,31,30,29,28,27,25,24,
                    23,21,19,18,16,14,13,11,9,8,7,5,4,3,2,1,1,0,0,0
                };

                uint8_t brightness =
                    layer_breath[((now % 3000) * 60) / 3000] * 13 / 32;

                if (s->layer == 0) {
                    f.outer.r = brightness;
                } else {
                    f.outer.b = brightness;
                }
            } else {
                f.outer = status_layer_color(s->layer);
            }
        }
    } else if (s->profile_valid) {
        /* Show the selected slot continuously while active, even disconnected.
         * This indicates selection, not whether the host link is up. */
        f.outer = status_profile_color(s->profile);
    }

    return f;
}
