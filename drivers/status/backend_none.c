/* SPDX-License-Identifier: MIT
 * The XIAO central broadcasts status to the Cornix LEDs; no local RGB strip.
 */
#include "backend.h"
int status_backend_init(void) { return 0; }
int status_backend_write(struct status_frame frame, int64_t now) {
    (void)frame; (void)now; return 0;
}
