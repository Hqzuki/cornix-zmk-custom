# SPDX-License-Identifier: MIT
# The short-name behavior exists in normal and reset images; reset is a no-op.
target_sources(app PRIVATE sync_behavior.c)
if(CONFIG_COREX_LED_STATUS)
  target_sources(app PRIVATE controller.c)
  if(CONFIG_BOARD_COREX_RIGHT)
    target_sources(app PRIVATE backend_pwm.c)
  else()
    target_sources(app PRIVATE backend_strip.c)
  endif()
endif()
