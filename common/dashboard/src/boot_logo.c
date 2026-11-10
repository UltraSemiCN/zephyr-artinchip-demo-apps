/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

#include "dashboard.h"

LOG_MODULE_REGISTER(dashboard_boot_logo);

/* LVGL is initialized at priority 95 in all four dashboard profiles. */
BUILD_ASSERT(CONFIG_LV_Z_INIT_PRIORITY < 96, "Boot logo must follow LVGL initialization");
BUILD_ASSERT(!IS_ENABLED(CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE),
	     "The dashboard owns LVGL rendering");

static int dashboard_boot_logo_init(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	static const char *const websites[] = {"www.artinchip.com", "www.ultrasemi.com"};
	struct display_capabilities caps;
	lv_obj_t *screen;
	int ret;

	if (!device_is_ready(display) || lv_display_get_default() == NULL) {
		return 0;
	}
	display_get_capabilities(display, &caps);
	if (caps.x_resolution != dashboard_layout.width ||
	    caps.y_resolution != dashboard_layout.height) {
		return 0;
	}

	screen = lv_screen_active();
	lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	lv_obj_set_scrollable(screen, false);

	for (size_t i = 0; i < ARRAY_SIZE(websites); i++) {
		int32_t center_x = (int32_t)caps.x_resolution / 2 - 120 + (int32_t)i * 240;
		lv_obj_t *logo = lv_image_create(screen);
		lv_obj_t *website = lv_label_create(screen);

		lv_image_set_src(logo, dashboard_layout.logos[i]);
		lv_obj_align(logo, LV_ALIGN_CENTER, center_x - caps.x_resolution / 2, -16);
		lv_label_set_text_static(website, websites[i]);
		lv_obj_set_style_text_color(website, lv_color_hex(0xe0e6ed), 0);
		lv_obj_set_style_text_font(website, &lv_font_montserrat_18, 0);
		lv_obj_align(website, LV_ALIGN_CENTER, center_x - caps.x_resolution / 2, 22);
	}

	/* Flush the logo before enabling scanout; no artificial minimum dwell time. */
	lv_refr_now(NULL);
	ret = display_blanking_off(display);
	if (ret != 0 && ret != -ENOSYS) {
		LOG_WRN("Boot logo output failed: %d", ret);
	}
	/* Failure to show a logo must not prevent normal application startup. */
	return 0;
}

SYS_INIT(dashboard_boot_logo_init, POST_KERNEL, 96);
