# SPDX-License-Identifier: MIT
zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/../../include)
if(CONFIG_COREX_CORON_KEYMAP AND CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE default_layer.c)
endif()
