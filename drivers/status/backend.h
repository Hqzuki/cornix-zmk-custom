/* SPDX-License-Identifier: MIT */
#pragma once
#include "patterns.h"
int status_backend_init(void);
int status_backend_write(struct status_frame frame, int64_t now);
int corex_status_receive(uint32_t profile, uint32_t flags);
