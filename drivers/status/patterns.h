/* SPDX-License-Identifier: MIT
 * Shared CoreX/Cornix status patterns. Timing and breathing LUT follow the
 * Cornix indicator, Copyright (c) 2026 numachang.
 * Pure state/render logic: also compiled by the host regression tests.
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
    int64_t profile_since, low_since, full_since, charge_since;
    bool powered, full_shown;
    /* CoreX right: outer LED shows the active layer and the inner LED only
     * this half's battery/charging; the split link is shown on the left. */
    bool layer_mode;
    uint8_t layer;
};
#define STATUS_STATE_INIT {.low_since = -1, .full_since = -1}
static inline void status_peer(struct status_state *s, bool connected, int64_t now) {
    if (!s->peer_known || s->peer_connected != connected) {
        s->peer_known = true; s->peer_connected = connected; s->peer_since = now;
    }
}
static inline bool status_profile(struct status_state *s, uint8_t index, bool connected,
                                  bool open, int64_t now) {
    if (s->profile_valid && s->profile == index && s->host_connected == connected &&
        s->host_open == open) { return false; }
    s->profile_valid = true; s->profile = index;
    s->host_connected = connected; s->host_open = open; s->profile_since = now;
    return true;
}
static inline void status_battery(struct status_state *s, uint8_t level, int64_t now) {
    if (level == 0 || level > 100) { return; }
    s->battery = level;
    if (level <= 20) { s->low_since = now; }
    if (s->powered && level >= 95 && !s->full_shown) {
        s->full_shown = true; s->full_since = now;
    } else if (s->powered && level < 95 && s->full_shown) {
        s->full_shown = false; s->full_since = -1; s->charge_since = now;
    }
}
static inline void status_power(struct status_state *s, bool powered, int64_t now) {
    if (s->powered == powered) { return; }
    s->powered = powered; s->charge_since = now;
    s->full_shown = powered && s->battery >= 95;
    s->full_since = s->full_shown ? now : -1;
}
/* Host BT profiles 0-4 (BLUETOOTH layer selects all five). */
static inline struct status_rgb status_profile_color(uint8_t profile) {
    static const struct status_rgb colors[] = {
        {0,16,0}, {16,0,0}, {0,0,16}, {16,10,0}, {12,0,16},
    };
    return profile < 5 ? colors[profile] : (struct status_rgb){8,8,8};
}
/* Layer ids follow config/corex_right.keymap definition order. */
static inline struct status_rgb status_layer_color(uint8_t layer) {
    static const struct status_rgb colors[] = {
        {0,0,16},   /* 0 WIN: blue */
        {10,10,10}, /* 1 MAC: white */
        {16,10,0},  /* 2 NUM: yellow */
        {12,0,16},  /* 3 FN: purple */
        {0,16,0},   /* 4 PAN_WIN: green */
        {0,16,0},   /* 5 PAN_MAC: green */
        {0,12,12},  /* 6 GESTURE: cyan */
        {16,4,0},   /* 7 MOUSE: orange */
        {16,0,0},   /* 8 BLUETOOTH: red */
    };
    return layer < 9 ? colors[layer] : (struct status_rgb){0};
}
static inline struct status_frame status_render(const struct status_state *s, int64_t now) {
    struct status_frame f = {0};
    const struct status_rgb red = {16,0,0}, blue = {0,0,16}, green = {0,16,0};
    int64_t low = now - s->low_since, peer = now - s->peer_since;
    /* Reserve the whole low-battery pattern, including its dark gaps. */
    if (s->low_since >= 0 && low >= 0 && low < 1200) {
        if (low < 400 || low >= 800) { f.inner = red; }
    } else if (!s->layer_mode && s->peer_known && peer >= 0 &&
               peer < (s->peer_connected ? 3000 : 10000)) {
        if (s->peer_connected || peer % 1000 < 400) { f.inner = blue; }
    } else if (s->powered) {
        if (s->full_shown) {
            if (now - s->full_since < 3000) { f.inner = green; }
        } else {
            static const uint8_t breath[60] = {
                0,0,0,1,1,2,3,4,5,7,8,9,11,13,14,16,18,19,21,23,
                24,25,27,28,29,30,31,31,32,32,32,32,31,31,30,29,28,27,25,24,
                23,21,19,18,16,14,13,11,9,8,7,5,4,3,2,1,1,0,0,0
            };
            f.inner.g = breath[((now - s->charge_since) % 2000) * 60 / 2000];
        }
    }
    if (s->layer_mode) {
        f.outer = status_layer_color(s->layer);
    } else if (s->profile_valid) {
        int64_t host = now - s->profile_since;
        bool on = s->host_connected || !s->host_open ? host < 600 :
                  host < 10000 && host % 1000 < 400;
        if (host >= 0 && on) {
            f.outer = status_profile_color(s->profile);
        }
    }
    return f;
}
