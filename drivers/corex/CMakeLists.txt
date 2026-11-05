# SPDX-License-Identifier: MIT
zephyr_library()
zephyr_library_sources(board.c)
zephyr_library_sources_ifdef(CONFIG_COREX_PAW3222 paw3222.c)
