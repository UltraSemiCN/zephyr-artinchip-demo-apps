# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(graphics_validation_d133_lvds)
if(NOT BOARD STREQUAL "artinchip_d13x_demo88_nor")
  message(FATAL_ERROR "This application requires artinchip_d13x_demo88_nor/d13x")
endif()
target_sources(app PRIVATE src/main.c)
