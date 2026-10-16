/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dashboard.h"
#include "assets/dashboard_logos.h"

const struct dashboard_layout dashboard_layout = {
	.width = 1024,
	.height = 600,
	.header = {552, 16, 456, 172},
	.chip_name = DASHBOARD_CHIP_NAME,
	.logos = {&dashboard_artinchip_logo, &dashboard_ultrasemi_logo},
	.gauges = {
		{16, 16, 252, 276},
		{284, 16, 252, 276},
		{16, 308, 252, 276},
		{284, 308, 252, 276},
	},
	.chart = {552, 204, 456, 172},
	.meters = {
		{552, 402, 280, 50},
		{552, 468, 280, 50},
		{552, 534, 280, 50},
	},
	.lamps = {848, 392, 160, 192},
};
