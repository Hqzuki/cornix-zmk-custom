# SPDX-License-Identifier: MIT
# Share ZMK application headers and generated includes (event manager, BLE).
target_sources(app PRIVATE board.c)
if(CONFIG_COREX_PAW3222)
  target_sources(app PRIVATE paw3222.c)
endif()
