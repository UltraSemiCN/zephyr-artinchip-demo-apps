# SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
# SPDX-License-Identifier: Apache-2.0

set(DASHBOARD_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(app PRIVATE ${DASHBOARD_COMMON_DIR}/src)
target_sources(app PRIVATE
  ${DASHBOARD_COMMON_DIR}/src/main.c
  ${DASHBOARD_COMMON_DIR}/src/dashboard.c
)

if(CONFIG_LVGL_FS_RESOURCES)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)

  set(LVGL_RESOURCES_DIR ${DASHBOARD_COMMON_DIR}/resources)
  set(LVGL_DATA_LFS ${CMAKE_BINARY_DIR}/zephyr/data.lfs)
  set(LVGL_GEN_LITTLEFS_SCRIPT ${DASHBOARD_COMMON_DIR}/scripts/gen_littlefs_data.py)

  file(GLOB LVGL_RESOURCE_FILES CONFIGURE_DEPENDS
       ${LVGL_RESOURCES_DIR}/*)

  add_custom_command(
    OUTPUT ${LVGL_DATA_LFS}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/zephyr
    COMMAND ${Python3_EXECUTABLE} ${LVGL_GEN_LITTLEFS_SCRIPT}
            --source ${LVGL_RESOURCES_DIR}
            --output ${LVGL_DATA_LFS}
    DEPENDS ${LVGL_GEN_LITTLEFS_SCRIPT} ${LVGL_RESOURCE_FILES}
    COMMENT "Generating LittleFS data partition from resources/"
    VERBATIM
  )

  add_custom_target(lvgl_data_lfs DEPENDS ${LVGL_DATA_LFS})
  add_dependencies(zephyr_final lvgl_data_lfs)
endif()
