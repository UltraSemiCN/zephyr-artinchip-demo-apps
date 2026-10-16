/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display/artinchip_de.h>
#include <zephyr/drivers/display/ek79007ad.h>
#include <zephyr/drivers/mipi_dsi/artinchip_dsi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dashboard_display, CONFIG_LOG_DEFAULT_LEVEL);

BUILD_ASSERT(DT_PROP(DT_CHOSEN(zephyr_display), framebuffer_address) +
	     DT_PROP(DT_CHOSEN(zephyr_display), framebuffer_size) <=
	     DT_REG_ADDR(DT_CHOSEN(zephyr_sram)), "Framebuffer overlaps application RAM");

#if defined(CONFIG_EK79007AD)
static int dashboard_display_init(void)
{
	const struct device *dsi = DEVICE_DT_GET(DT_NODELABEL(mipi_dsi));
	const struct device *panel = DEVICE_DT_GET(DT_NODELABEL(panel_7inch));
	const struct device *de = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	int ret;

	if (!device_is_ready(dsi) || !device_is_ready(panel) || !device_is_ready(de)) {
		return -ENODEV;
	}

	ret = ek79007ad_attach_dsi(panel);
	if (ret == 0) {
		ret = artinchip_dsi_prepare(dsi);
	}
	if (ret == 0) {
		ret = ek79007ad_bringup(panel);
	}
	if (ret == 0) {
		ret = artinchip_de_bringup(de, 0U);
	}
	if (ret == 0) {
		ret = ek79007ad_backlight_enable(panel);
	}
	if (ret != 0) {
		LOG_ERR("Display bring-up failed: %d", ret);
	}
	return ret;
}

SYS_INIT(dashboard_display_init, POST_KERNEL, 90);

#endif /* CONFIG_EK79007AD */
