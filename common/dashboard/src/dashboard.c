/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi Technology Development Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dashboard.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(dashboard, CONFIG_LOG_DEFAULT_LEVEL);

#define GAUGE_SIZE        210
#define GAUGE_NEEDLE_LEN  72
#define CHART_POINTS      40
#define TICK_MS           80

struct gauge_panel {
	lv_obj_t *scale;
	lv_obj_t *needle;
	lv_obj_t *value;
	int32_t min;
	int32_t max;
	int32_t value_i;
	int32_t needle_len;
	uint32_t phase;
	const char *unit;
	lv_color_t accent;
};

struct dashboard_ctx {
	struct gauge_panel speed;
	struct gauge_panel rpm;
	struct gauge_panel temp;
	struct gauge_panel power;
	lv_obj_t *chart;
	lv_chart_series_t *chart_series;
	lv_obj_t *bar_cpu;
	lv_obj_t *bar_mem;
	lv_obj_t *bar_net;
	lv_obj_t *led_run;
	lv_obj_t *led_link;
	lv_obj_t *led_alert;
	lv_obj_t *clock;
	lv_timer_t *timer;
	uint32_t tick;
};

static struct dashboard_ctx dash;

static void style_panel_bg(lv_obj_t *obj)
{
	lv_obj_set_style_bg_color(obj, lv_color_hex(0x0d1b2a), 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(obj, 0, 0);
	lv_obj_set_style_pad_all(obj, 0, 0);
}

static void style_card(lv_obj_t *obj, lv_color_t accent)
{
	lv_obj_set_style_bg_color(obj, lv_color_hex(0x1b2838), 0);
	lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x243b55), 0);
	lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(obj, 14, 0);
	lv_obj_set_style_border_color(obj, accent, 0);
	lv_obj_set_style_border_width(obj, 2, 0);
	lv_obj_set_style_shadow_width(obj, 18, 0);
	lv_obj_set_style_shadow_color(obj, accent, 0);
	lv_obj_set_style_shadow_opa(obj, LV_OPA_30, 0);
}

static lv_obj_t *create_header(lv_obj_t *parent, const struct dashboard_layout *layout)
{
	lv_obj_t *bar = lv_obj_create(parent);

	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, layout->header.width, layout->header.height);
	lv_obj_set_pos(bar, layout->header.x, layout->header.y);
	lv_obj_set_style_bg_color(bar, lv_color_hex(0x111d2e), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
	lv_obj_set_style_border_color(bar, lv_color_hex(0x00d4ff), 0);
	lv_obj_set_style_border_width(bar, 2, 0);
	lv_obj_set_scrollable(bar, false);

	if (layout->chip_name != NULL) {
		lv_obj_t *title = lv_label_create(bar);

		lv_label_set_text_fmt(title, "%s Zephyr LVGL Dashboard", layout->chip_name);
		lv_obj_set_width(title, layout->header.width - 24);
		lv_obj_set_style_text_color(title, lv_color_hex(0x00d4ff), 0);
		lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
		lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
	} else {
		lv_obj_t *title = lv_label_create(bar);

		lv_label_set_text(title, "ArtInChip LVGL Dashboard (Zephyr OS)");
		lv_obj_set_style_text_color(title, lv_color_hex(0x00d4ff), 0);
		lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
		lv_obj_align(title, LV_ALIGN_LEFT_MID, 20, 0);
	}

	if (layout->chip_name != NULL) {
		const char *websites[] = {"www.artinchip.com", "www.ultrasemi.com"};
		const int32_t column_width = (layout->header.width - 24) / 2;

		for (size_t i = 0; i < ARRAY_SIZE(layout->logos); i++) {
			const int32_t column_x = 12 + (int32_t)i * column_width;
			lv_obj_t *logo = lv_image_create(bar);
			lv_obj_t *website = lv_label_create(bar);

			lv_image_set_src(logo, layout->logos[i]);
			lv_obj_set_pos(logo,
				       column_x + (column_width - layout->logos[i]->header.w) / 2,
				       62);
			lv_label_set_text(website, websites[i]);
			lv_obj_set_style_text_font(website, &lv_font_montserrat_14, 0);
			lv_obj_set_style_text_color(website, lv_color_hex(0xe0e1dd), 0);
			lv_obj_set_width(website, column_width);
			lv_obj_set_style_text_align(website, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_pos(website, column_x, 122);
		}

	} else {
		dash.clock = lv_label_create(bar);
		lv_obj_set_style_text_color(dash.clock, lv_color_hex(0xe0e1dd), 0);
		lv_obj_set_style_text_font(dash.clock, &lv_font_montserrat_18, 0);
		lv_obj_align(dash.clock, LV_ALIGN_RIGHT_MID, -20, 0);
	}

	return bar;
}

static void gauge_apply_value(struct gauge_panel *g)
{
	char buf[24];

	lv_scale_set_line_needle_value(g->scale, g->needle, g->needle_len, g->value_i);
	snprintf(buf, sizeof(buf), "%d %s", (int)g->value_i, g->unit);
	lv_label_set_text(g->value, buf);
}

static lv_obj_t *create_gauge(lv_obj_t *parent, struct gauge_panel *g, const char *title,
			      int32_t min, int32_t max, const char *unit, lv_color_t accent,
			      const struct dashboard_rect *rect)
{
	const int width = rect->width;
	const int scale_size = MIN(width * GAUGE_SIZE / (GAUGE_SIZE + 24),
				   rect->height * GAUGE_SIZE / (GAUGE_SIZE + 56));
	lv_obj_t *card = lv_obj_create(parent);
	lv_obj_t *title_lbl;

	g->min = min;
	g->max = max;
	g->unit = unit;
	g->accent = accent;
	g->value_i = min;
	g->phase = 0U;
	g->needle_len = scale_size * GAUGE_NEEDLE_LEN / GAUGE_SIZE;

	lv_obj_remove_style_all(card);
	lv_obj_set_size(card, width, rect->height);
	lv_obj_set_pos(card, rect->x, rect->y);
	style_card(card, accent);
	lv_obj_set_scrollable(card, false);

	title_lbl = lv_label_create(card);
	lv_label_set_text(title_lbl, title);
	lv_obj_set_style_text_color(title_lbl, accent, 0);
	lv_obj_set_style_text_font(title_lbl, width > GAUGE_SIZE + 24 ?
				    &lv_font_montserrat_18 : &lv_font_montserrat_14, 0);
	lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, 10);

	g->scale = lv_scale_create(card);
	lv_obj_set_size(g->scale, scale_size, scale_size);
	lv_obj_align(g->scale, LV_ALIGN_CENTER, 0, 0);
	lv_scale_set_mode(g->scale, LV_SCALE_MODE_ROUND_INNER);
	lv_scale_set_range(g->scale, min, max);
	lv_scale_set_total_tick_count(g->scale, 41);
	lv_scale_set_major_tick_every(g->scale, 5);
	lv_scale_set_label_show(g->scale, true);
	lv_scale_set_angle_range(g->scale, 270);
	lv_scale_set_rotation(g->scale, 135);
	lv_obj_set_style_bg_color(g->scale, lv_color_hex(0x0a111b), 0);
	lv_obj_set_style_bg_grad_color(g->scale, lv_color_hex(0x1a2a3a), 0);
	lv_obj_set_style_bg_grad_dir(g->scale, LV_GRAD_DIR_VER, 0);
	lv_obj_set_style_bg_opa(g->scale, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(g->scale, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_line_color(g->scale, accent, LV_PART_ITEMS);
	lv_obj_set_style_line_color(g->scale, lv_color_hex(0xffffff), LV_PART_INDICATOR);
	lv_obj_set_style_length(g->scale, 4, LV_PART_ITEMS);
	lv_obj_set_style_length(g->scale, 10, LV_PART_INDICATOR);
	lv_obj_set_style_text_color(g->scale, lv_color_hex(0xc8d6e5), LV_PART_MAIN);

	g->needle = lv_line_create(g->scale);
	lv_obj_set_style_line_width(g->needle, 5, LV_PART_MAIN);
	lv_obj_set_style_line_color(g->needle, accent, LV_PART_MAIN);
	lv_obj_set_style_line_rounded(g->needle, true, LV_PART_MAIN);

	g->value = lv_label_create(card);
	lv_obj_set_style_text_color(g->value, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_text_font(g->value, width > GAUGE_SIZE + 24 ?
				    &lv_font_montserrat_24 : &lv_font_montserrat_18, 0);
	lv_obj_align(g->value, LV_ALIGN_BOTTOM_MID, 0, -10);

	gauge_apply_value(g);
	return card;
}

static lv_obj_t *create_chart(lv_obj_t *parent, int x, int y, int w, int h)
{
	lv_obj_t *card = lv_obj_create(parent);
	lv_obj_t *title;
	static const int32_t seed[] = { 12, 18, 25, 32, 28, 40, 55, 48, 62, 58 };
	int chart_w = w - 48;
	int chart_h = h - 56;

	if (chart_w < 120) {
		chart_w = w - 24;
	}
	if (chart_h < 80) {
		chart_h = h - 40;
	}

	lv_obj_remove_style_all(card);
	lv_obj_set_size(card, w, h);
	lv_obj_align(card, LV_ALIGN_TOP_LEFT, x, y);
	style_card(card, lv_color_hex(0x7b2cbf));
	lv_obj_set_scrollable(card, false);

	title = lv_label_create(card);
	lv_label_set_text(title, "Live Signal");
	lv_obj_set_style_text_color(title, lv_color_hex(0xcc8aff), 0);
	lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 8);

	if (dashboard_layout.chip_name != NULL) {
		dash.clock = lv_label_create(card);
		lv_obj_set_style_text_color(dash.clock, lv_color_hex(0xe0e1dd), 0);
		lv_obj_set_style_text_font(dash.clock, &lv_font_montserrat_14, 0);
		lv_obj_align(dash.clock, LV_ALIGN_TOP_RIGHT, -12, 8);
	}

	dash.chart = lv_chart_create(card);
	lv_obj_set_size(dash.chart, chart_w, chart_h);
	lv_obj_align(dash.chart, LV_ALIGN_BOTTOM_MID, 0, -10);
	lv_chart_set_type(dash.chart, LV_CHART_TYPE_LINE);
	lv_chart_set_point_count(dash.chart, CHART_POINTS);
	lv_chart_set_axis_min_value(dash.chart, LV_CHART_AXIS_PRIMARY_Y, 0);
	lv_chart_set_axis_max_value(dash.chart, LV_CHART_AXIS_PRIMARY_Y, 100);
	lv_obj_set_style_bg_color(dash.chart, lv_color_hex(0x0f1724), 0);
	lv_obj_set_style_line_width(dash.chart, 3, LV_PART_ITEMS);

	dash.chart_series = lv_chart_add_series(dash.chart, lv_color_hex(0x00f5d4),
						LV_CHART_AXIS_PRIMARY_Y);
	lv_chart_set_series_values(dash.chart, dash.chart_series, seed,
				   ARRAY_SIZE(seed));

	return card;
}

static lv_obj_t *create_meter_bar(lv_obj_t *parent, const char *name, lv_color_t color,
				  lv_align_t align, int x_ofs, int y_ofs, int width,
				  lv_obj_t **bar_out)
{
	lv_obj_t *wrap = lv_obj_create(parent);
	lv_obj_t *lbl;
	lv_obj_t *bar;

	lv_obj_remove_style_all(wrap);
	lv_obj_set_size(wrap, width, 50);
	lv_obj_align(wrap, align, x_ofs, y_ofs);
	lv_obj_set_style_bg_opa(wrap, LV_OPA_TRANSP, 0);
	lv_obj_set_scrollable(wrap, false);

	lbl = lv_label_create(wrap);
	lv_label_set_text(lbl, name);
	lv_obj_set_style_text_color(lbl, color, 0);
	lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
	lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 0);

	bar = lv_bar_create(wrap);
	lv_obj_set_size(bar, width, 24);
	lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
	lv_bar_set_range(bar, 0, 100);
	lv_obj_set_style_bg_color(bar, lv_color_hex(0x1e2d3d), LV_PART_MAIN);
	lv_obj_set_style_bg_color(bar, color, LV_PART_INDICATOR);
	lv_obj_set_style_border_width(bar, 2, LV_PART_MAIN);
	lv_obj_set_style_border_color(bar, color, LV_PART_MAIN);
	lv_obj_set_style_border_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_pad_all(bar, 3, LV_PART_MAIN);
	lv_obj_set_style_radius(bar, 8, LV_PART_MAIN);
	lv_obj_set_style_radius(bar, 8, LV_PART_INDICATOR);

	*bar_out = bar;
	return wrap;
}

static lv_obj_t *create_status_leds(lv_obj_t *parent, int x, int y, int width, int height)
{
	lv_obj_t *card = lv_obj_create(parent);
	lv_obj_t **leds[] = {&dash.led_alert, &dash.led_link, &dash.led_run};
	const uint32_t colors[] = {0xff4057, 0xffc857, 0x36e58c};
	const bool vertical = height > width;

	lv_obj_remove_style_all(card);
	lv_obj_set_size(card, width, height);
	lv_obj_align(card, LV_ALIGN_TOP_LEFT, x, y);
	style_card(card, lv_color_hex(0x52677d));
	lv_obj_set_scrollable(card, false);

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		int offset = ((int)i - 1) * (vertical ? 50 : 54);

		*leds[i] = lv_led_create(card);
		lv_obj_set_size(*leds[i], 28, 28);
		lv_obj_align(*leds[i], LV_ALIGN_CENTER, vertical ? 0 : offset,
			     vertical ? offset : 0);
		lv_led_set_color(*leds[i], lv_color_hex(colors[i]));
		lv_led_set_brightness(*leds[i], i == 0 ? 255 : 40);
	}

	return card;
}

static int32_t wave_value(uint32_t tick, uint32_t phase, int32_t min, int32_t max)
{
	uint32_t span = (uint32_t)(max - min);
	uint32_t angle = (tick * 7U + phase) % 360U;
	int32_t sinv = lv_trigo_sin(angle);

	return min + (int32_t)((span * (sinv + LV_TRIGO_SIN_MAX)) /
			       (2 * LV_TRIGO_SIN_MAX));
}

static void dashboard_timer_cb(lv_timer_t *timer)
{
	char time_buf[16];
	int32_t cpu;
	int32_t mem;
	int32_t net;
	int32_t chart_val;

	ARG_UNUSED(timer);

	dash.tick++;
	dash.speed.value_i = wave_value(dash.tick, 0U, dash.speed.min, dash.speed.max);
	dash.rpm.value_i = wave_value(dash.tick, 90U, dash.rpm.min, dash.rpm.max);
	dash.temp.value_i = wave_value(dash.tick, 180U, dash.temp.min, dash.temp.max);
	dash.power.value_i = wave_value(dash.tick, 270U, dash.power.min, dash.power.max);

	gauge_apply_value(&dash.speed);
	gauge_apply_value(&dash.rpm);
	gauge_apply_value(&dash.temp);
	gauge_apply_value(&dash.power);

	cpu = 35 + (dash.tick * 3U) % 55;
	mem = 25 + (dash.tick * 5U) % 60;
	net = 10 + (dash.tick * 7U) % 80;
	lv_bar_set_value(dash.bar_cpu, cpu, LV_ANIM_ON);
	lv_bar_set_value(dash.bar_mem, mem, LV_ANIM_ON);
	lv_bar_set_value(dash.bar_net, net, LV_ANIM_ON);

	chart_val = wave_value(dash.tick, 45U, 5, 95);
	lv_chart_set_next_value(dash.chart, dash.chart_series, chart_val);

	snprintf(time_buf, sizeof(time_buf), "T+%u s", dash.tick * TICK_MS / 1000U);
	lv_label_set_text(dash.clock, time_buf);

	/* Advance the red / yellow / green lamp every 480 ms. */
	uint32_t lamp = (dash.tick / 6U) % 3U;

	lv_led_set_brightness(dash.led_alert, lamp == 0U ? 255 : 40);
	lv_led_set_brightness(dash.led_link, lamp == 1U ? 255 : 40);
	lv_led_set_brightness(dash.led_run, lamp == 2U ? 255 : 40);
}

void dashboard_create(void)
{
	lv_obj_t *screen = lv_screen_active();
	const struct dashboard_layout *layout = &dashboard_layout;
	lv_obj_t **bars[] = {&dash.bar_cpu, &dash.bar_mem, &dash.bar_net};
	static const char *const names[] = {"CPU LOAD", "MEMORY", "NETWORK"};
	const uint32_t colors[] = {0x00d4ff, 0xff9e00, 0x06d6a0};

	style_panel_bg(screen);
	lv_obj_set_scrollable(screen, false);
	create_header(screen, layout);
	create_gauge(screen, &dash.speed, "SPEED", 0, 180, "km/h",
		     lv_color_hex(0x00d4ff), &layout->gauges[0]);
	create_gauge(screen, &dash.rpm, "ENGINE", 800, 7000, "rpm",
		     lv_color_hex(0xff9e00), &layout->gauges[1]);
	create_gauge(screen, &dash.temp, "TEMP", 20, 120, "C",
		     lv_color_hex(0xff006e), &layout->gauges[2]);
	create_gauge(screen, &dash.power, "POWER", 0, 100, "%",
		     lv_color_hex(0x80ed99), &layout->gauges[3]);
	create_chart(screen, layout->chart.x, layout->chart.y,
		     layout->chart.width, layout->chart.height);
	create_status_leds(screen, layout->lamps.x, layout->lamps.y,
			   layout->lamps.width, layout->lamps.height);
	for (size_t i = 0; i < ARRAY_SIZE(bars); i++) {
		const struct dashboard_rect *rect = &layout->meters[i];

		create_meter_bar(screen, names[i], lv_color_hex(colors[i]),
				 LV_ALIGN_TOP_LEFT, rect->x, rect->y, rect->width, bars[i]);
	}

	dash.timer = lv_timer_create(dashboard_timer_cb, TICK_MS, NULL);
}
