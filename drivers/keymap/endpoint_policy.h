/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
/* Policy slots: 0 = USB, 1+n = Bluetooth profile n. These are routing
 * decisions only; the existing persisted ZMK endpoint indices are unchanged.
 * A disconnected keyboard follows its selected BLE profile, not USB priority.
 */
static inline int default_layer_target_slot(bool usb_selected, int active_profile,
                                           bool explicit_ble) {
    if (usb_selected && !explicit_ble) { return 0; }
    return active_profile >= 0 ? active_profile + 1 : -1;
}
