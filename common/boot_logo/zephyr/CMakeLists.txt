# SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MCUBOOT)
  zephyr_library()
  zephyr_library_sources(../boot_logo.c ../image.c)
  zephyr_library_sources_ifdef(CONFIG_EK79007AD ../../dashboard/src/display_init.c)
endif()
