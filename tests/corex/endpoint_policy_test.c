/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "../../drivers/keymap/endpoint_policy.h"
int main(void) {
    /* USB priority is irrelevant when the actual output is BLE. */
    assert(default_layer_target_slot(false, 0, false) == 1);
    assert(default_layer_target_slot(false, 4, false) == 5);
    /* BT_SEL updates the profile before the old connection is replaced. */
    assert(default_layer_target_slot(false, 2, true) == 3);
    /* USB stays independent; BT macros must save BLE even with USB plugged in. */
    assert(default_layer_target_slot(true, 2, false) == 0);
    assert(default_layer_target_slot(true, 2, true) == 3);
    /* Disconnected startup/advertising still addresses the active BLE slot. */
    assert(default_layer_target_slot(false, 1, false) == 2);
    assert(default_layer_target_slot(false, -1, false) == -1);
    puts("Endpoint routing: BLE fallback, profile changes, USB isolation and offline selection passed");
}
