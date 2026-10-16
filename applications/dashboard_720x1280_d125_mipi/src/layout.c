/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dashboard.h"

const struct dashboard_layout dashboard_layout = {
	.width = 720,
	.height = 1280,
	.header = {0, 0, 720, 52},
	.gauges = {
		{16, 68, 336, 381},
		{368, 68, 336, 381},
		{16, 465, 336, 381},
		{368, 465, 336, 381},
	},
	.chart = {16, 862, 688, 212},
	.meters = {
		{16, 1090, 512, 50},
		{16, 1152, 512, 50},
		{16, 1214, 512, 50},
	},
	.lamps = {544, 1090, 160, 174},
};
