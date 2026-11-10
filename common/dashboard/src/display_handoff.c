/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dashboard_handoff);

BUILD_ASSERT(CONFIG_DISPLAY_INIT_PRIORITY > 79,
	     "Reset inherited scanout before display initialization");
#if DT_NODE_HAS_STATUS(DT_NODELABEL(mipi_dsi), okay)
BUILD_ASSERT(CONFIG_MIPI_DSI_INIT_PRIORITY > 79,
	     "Reset inherited DSI state before host initialization");
#endif

static int dashboard_display_handoff(void)
{
	static const struct reset_dt_spec resets[] = {
		/* Stop the pixel producer before resetting the DSI consumer. */
		RESET_DT_SPEC_GET(DT_CHOSEN(zephyr_display)),
#if DT_NODE_HAS_STATUS(DT_NODELABEL(mipi_dsi), okay)
		RESET_DT_SPEC_GET(DT_NODELABEL(mipi_dsi)),
#endif
	};
	int ret;

	/* MCUboot leaves a live scanout. Software driver state starts empty, so
	 * deasserting reset later cannot discard its FIFOs or latched timing.
	 * Hold reset until each driver explicitly starts its own hardware.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(resets); i++) {
		if (!device_is_ready(resets[i].dev)) {
			LOG_ERR("Display reset controller is not ready");
			return -ENODEV;
		}
		ret = reset_line_assert_dt(&resets[i]);
		if (ret != 0) {
			LOG_ERR("Display handoff reset %u failed: %d", (unsigned int)i, ret);
			return ret;
		}
	}
	return 0;
}

SYS_INIT(dashboard_display_handoff, POST_KERNEL, 79);
