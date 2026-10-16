# SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dashboard_1024x600_d125_mipi)

if(NOT BOARD STREQUAL "artinchip_d12p_demo88_nor")
  message(FATAL_ERROR "This application requires artinchip_d12p_demo88_nor/d125ccr")
endif()

include(../../common/dashboard/application.cmake)
target_sources(app PRIVATE
  ../../common/dashboard/src/layout_1024x600.c
  ../../common/dashboard/src/assets/artinchip_logo.c
  ../../common/dashboard/src/assets/ultrasemi_logo.c
)
target_sources(app PRIVATE ../../common/dashboard/src/display_init.c)

target_compile_definitions(app PRIVATE DASHBOARD_CHIP_NAME="D125")
