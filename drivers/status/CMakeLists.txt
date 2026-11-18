# SPDX-License-Identifier: MIT
# The short-name behavior exists in normal and reset images; reset is a no-op.
target_sources(app PRIVATE sync_behavior.c)
if(CONFIG_CORNIX_LED_STATUS)
  target_sources(app PRIVATE controller.c backend_strip.c)
endif()
