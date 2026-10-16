# SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dashboard_720x1280_d125_mipi)

if(NOT BOARD STREQUAL "ultrasemi_d12p_usbdisp_demo88_nor_5_5")
  message(FATAL_ERROR "This application requires ultrasemi_d12p_usbdisp_demo88_nor_5_5/d125ccv")
endif()

include(../../common/dashboard/application.cmake)
target_sources(app PRIVATE src/layout.c)
