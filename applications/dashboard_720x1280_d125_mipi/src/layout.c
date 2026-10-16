/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dashboard.h"
#include "assets/dashboard_logos.h"

const struct dashboard_layout dashboard_layout = {
	.width = 720,
	.height = 1280,
	.header = {16, 16, 688, 172},
	.chip_name = "D125",
	.logos = {&dashboard_artinchip_logo, &dashboard_ultrasemi_logo},
	.gauges = {
		{16, 204, 336, 313},
		{368, 204, 336, 313},
		{16, 533, 336, 313},
		{368, 533, 336, 313},
	},
	.chart = {16, 862, 688, 212},
	.meters = {
		{16, 1090, 512, 50},
		{16, 1152, 512, 50},
		{16, 1214, 512, 50},
	},
	.lamps = {544, 1090, 160, 174},
};
