/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(boot_shell, LOG_LEVEL_INF);

#ifdef CONFIG_DISPLAY
int boot_display_test(void);
#endif

int main(void)
{
	LOG_INF("Zephyr boot test: %s", CONFIG_BOARD_TARGET);
#ifdef CONFIG_DISPLAY
	return boot_display_test();
#else
	LOG_INF("Type help to test the shell; display is not enabled in this image");
#endif
	return 0;
}
