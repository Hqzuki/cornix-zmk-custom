# SPDX-License-Identifier: MIT
if(CONFIG_CORNIX_KEYMAP AND CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE os_key.c)
  target_sources_ifdef(CONFIG_CORNIX_AUTO_OS app PRIVATE auto_os.c)
endif()
