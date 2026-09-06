if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL OR NOT CONFIG_ZMK_SPLIT)
  zephyr_library()
  zephyr_library_sources(src/behavior_mouse_mode.c)
endif()
