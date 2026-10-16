/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_test, LOG_LEVEL_INF);

static uint16_t row[1024];
static const uint16_t colors[] = {0xffff, 0xffe0, 0x07ff, 0x07e0,
				0xf81f, 0xf800, 0x001f, 0x0000};

int boot_display_test(void)
{
	const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	struct display_capabilities caps;
	struct display_buffer_descriptor desc = {0};
	uint16_t marker = 0;
	int ret;

	if (!device_is_ready(display)) {
		LOG_ERR("Display is not ready; shell remains available");
		return -ENODEV;
	}
	display_get_capabilities(display, &caps);
	if (caps.x_resolution == 0U || caps.x_resolution > ARRAY_SIZE(row) ||
	    caps.y_resolution == 0U) {
		return -ENOTSUP;
	}
	ret = display_set_pixel_format(display, PIXEL_FORMAT_RGB_565);
	if (ret != 0) {
		return ret;
	}
	desc.width = caps.x_resolution;
	desc.height = 1;
	desc.pitch = caps.x_resolution;
	desc.buf_size = caps.x_resolution * sizeof(row[0]);
	ret = display_blanking_off(display);
	if (ret != 0) {
		LOG_ERR("Display enable failed: %d", ret);
		return ret;
	}
	LOG_INF("LVDS test: %ux%u color bars with moving white marker; shell enabled",
		caps.x_resolution, caps.y_resolution);
	while (true) {
		for (uint16_t y = 0; y < caps.y_resolution; y++) {
			for (uint16_t x = 0; x < caps.x_resolution; x++) {
				bool border = x < 2U || x >= caps.x_resolution - 2U ||
					      y < 2U || y >= caps.y_resolution - 2U;

				row[x] = (border || x == marker) ? 0xffffU :
					 colors[x * ARRAY_SIZE(colors) / caps.x_resolution];
			}
			ret = display_write(display, 0, y, &desc, row);
			if (ret != 0) {
				LOG_ERR("Display write failed: %d", ret);
				return ret;
			}
		}
		marker = (marker + 16U) % caps.x_resolution;
		k_msleep(250);
	}
}
