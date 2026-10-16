# SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
# SPDX-License-Identifier: Apache-2.0

set(dashboard_ram_layout ram-layout.overlay)

# Application sysbuild fragments follow the board's default RAM layout.
# Both images must agree on the signed image load region.
foreach(image mcuboot ${DEFAULT_IMAGE})
  set(var "${image}_EXTRA_DTC_OVERLAY_FILE")
  if(image STREQUAL "${DEFAULT_IMAGE}" AND NOT DEFINED ${var})
    set(var EXTRA_DTC_OVERLAY_FILE)
  endif()
  sysbuild_cache_set(VAR ${var} APPEND REMOVE_DUPLICATES
                    "${CMAKE_CURRENT_LIST_DIR}/${dashboard_ram_layout}")
endforeach()
