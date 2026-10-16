/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include "dashboard.h"

LOG_MODULE_REGISTER(app);

int main(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	struct display_capabilities capabilities;
	int ret;

	if (!device_is_ready(display)) {
		LOG_ERR("Display is not ready");
		return 0;
	}

	display_get_capabilities(display, &capabilities);
	if (capabilities.x_resolution != dashboard_layout.width ||
	    capabilities.y_resolution != dashboard_layout.height) {
		LOG_ERR("Dashboard layout does not match display resolution");
		return 0;
	}

	dashboard_create();
	lv_timer_handler();
	ret = display_blanking_off(display);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return 0;
	}

	while (1) {
		lv_timer_handler();
		k_sleep(K_MSEC(10));
	}
}
