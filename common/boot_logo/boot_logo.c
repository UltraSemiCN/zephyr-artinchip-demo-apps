/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#define DISPLAY DT_CHOSEN(zephyr_display)
#define FB_START DT_PROP(DISPLAY, framebuffer_address)
#define FB_END (FB_START + DT_PROP(DISPLAY, framebuffer_size))
#define RAM DT_CHOSEN(zephyr_sram)
#define IMAGE_RAM DT_CHOSEN(mcuboot_image_ram)

BUILD_ASSERT(FB_END <= DT_REG_ADDR(RAM) ||
	     FB_START >= DT_REG_ADDR(RAM) + DT_REG_SIZE(RAM),
	     "Logo overlaps MCUboot memory");
BUILD_ASSERT(FB_END <= DT_REG_ADDR(IMAGE_RAM) ||
	     FB_START >= DT_REG_ADDR(IMAGE_RAM) + DT_REG_SIZE(IMAGE_RAM),
	     "Logo overlaps the loaded application");
BUILD_ASSERT(DT_PROP(DISPLAY, framebuffer_width) == 480 &&
	     DT_PROP(DISPLAY, framebuffer_height) == 96, "Unexpected boot logo canvas");

extern const uint16_t dashboard_boot_pixels[480 * 96];
LOG_MODULE_REGISTER(boot_logo);

static int boot_logo_init(void)
{
	const struct device *display = DEVICE_DT_GET(DISPLAY);
	const struct display_buffer_descriptor desc = {
		.width = 480,
		.height = 96,
		.pitch = 480,
		.buf_size = sizeof(dashboard_boot_pixels),
	};
	int ret;

	if (!device_is_ready(display)) {
		return 0;
	}
	ret = display_blanking_on(display);
	if (ret == 0) {
		ret = display_write(display, 0, 0, &desc, dashboard_boot_pixels);
	}
	if (ret == 0) {
		ret = display_blanking_off(display);
	}
	if (ret != 0) {
		LOG_WRN("Logo unavailable: %d; continuing boot", ret);
	}
	return 0;
}

SYS_INIT(boot_logo_init, POST_KERNEL, 96);
