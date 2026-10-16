/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARTINCHIP_DASHBOARD_H_
#define ARTINCHIP_DASHBOARD_H_

#include <lvgl.h>

struct dashboard_rect {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};

struct dashboard_layout {
	int32_t width;
	int32_t height;
	struct dashboard_rect header;
	const char *chip_name;
	const lv_image_dsc_t *logos[2];
	struct dashboard_rect gauges[4];
	struct dashboard_rect chart;
	struct dashboard_rect meters[3];
	struct dashboard_rect lamps;
};

extern const struct dashboard_layout dashboard_layout;
void dashboard_create(void);

#endif /* ARTINCHIP_DASHBOARD_H_ */
