# SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
# SPDX-License-Identifier: Apache-2.0

set_config_bool(${DEFAULT_IMAGE} CONFIG_DASHBOARD_BOOT_LOGO ${SB_CONFIG_DASHBOARD_BOOT_LOGO})
if(NOT SB_CONFIG_BOOTLOADER_MCUBOOT OR NOT SB_CONFIG_DASHBOARD_BOOT_LOGO)
  return()
endif()

sysbuild_cache_set(VAR mcuboot_ZEPHYR_EXTRA_MODULES APPEND REMOVE_DUPLICATES
                   "${CMAKE_CURRENT_LIST_DIR}")
sysbuild_cache_set(VAR mcuboot_EXTRA_DTC_OVERLAY_FILE APPEND REMOVE_DUPLICATES
                   "${APP_DIR}/boot-logo.overlay")
set_config_bool(mcuboot CONFIG_DISPLAY y)
set_config_bool(mcuboot CONFIG_GPIO y)
set_config_bool(mcuboot CONFIG_LVGL n)
if(BOARD STREQUAL "artinchip_d12p_demo88_nor")
  set_config_bool(mcuboot CONFIG_MIPI_DSI y)
  set_config_bool(mcuboot CONFIG_MIPI_DSI_ARTINCHIP_AUTO_ENABLE_SEQUENCE y)
  set_config_bool(mcuboot CONFIG_EK79007AD_DEFER_PANEL_INIT y)
elseif(BOARD STREQUAL "ultrasemi_d12p_usbdisp_demo88_nor_5_5")
  set_config_bool(mcuboot CONFIG_MIPI_DSI y)
endif()
