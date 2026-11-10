/* SPDX-License-Identifier: Apache-2.0 */
#define _POSIX_C_SOURCE 200809L
#include "dashboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint16_t *framebuffer;
static uint64_t start_ms;
static unsigned int flushes;

static uint64_t monotonic_ms(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (uint64_t)now.tv_sec * 1000U + now.tv_nsec / 1000000U;
}

static uint32_t ticks(void)
{
	return (uint32_t)(monotonic_ms() - start_ms);
}

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
	const int width = area->x2 - area->x1 + 1;
	const size_t stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_RGB565);

	for (int y = area->y1; y <= area->y2; y++) {
		memcpy(framebuffer + y * dashboard_layout.width + area->x1,
		       pixels + (y - area->y1) * stride, width * sizeof(uint16_t));
	}
	flushes++;
	lv_display_flush_ready(display);
}

static void save_frame(const char *directory, int index, uint32_t elapsed)
{
	char path[1024];

	snprintf(path, sizeof(path), "%s/frame_%02d_%05ums.ppm", directory, index, elapsed);
	FILE *file = fopen(path, "wb");

	if (file == NULL) {
		perror(path);
		exit(EXIT_FAILURE);
	}
	fprintf(file, "P6\n%d %d\n255\n", dashboard_layout.width, dashboard_layout.height);
	for (int i = 0; i < dashboard_layout.width * dashboard_layout.height; i++) {
		uint16_t pixel = framebuffer[i];
		uint8_t rgb[] = {
			((pixel >> 11) & 31) * 255 / 31,
			((pixel >> 5) & 63) * 255 / 63,
			(pixel & 31) * 255 / 31,
		};

		fwrite(rgb, sizeof(rgb), 1, file);
	}
	if (fclose(file) != 0) {
		perror("save_frame");
		exit(EXIT_FAILURE);
	}
	printf("%s elapsed_ms=%u flushes=%u\n", path, elapsed, flushes);
	fflush(stdout);
}

int main(int argc, char **argv)
{
	if (argc != 7) {
		fprintf(stderr, "usage: %s output_dir time1_ms ... time5_ms\n", argv[0]);
		return EXIT_FAILURE;
	}
	uint32_t capture_at[5];

	for (int i = 0; i < 5; i++) {
		capture_at[i] = strtoul(argv[i + 2], NULL, 10);
		if (capture_at[i] < 3000 || (i > 0 && capture_at[i] <= capture_at[i - 1])) {
			return EXIT_FAILURE;
		}
	}
	lv_init();
	start_ms = monotonic_ms();
	lv_tick_set_cb(ticks);
	lv_display_t *display = lv_display_create(dashboard_layout.width, dashboard_layout.height);
	size_t frame_size = dashboard_layout.width * dashboard_layout.height * sizeof(uint16_t);
	void *draw_buffer = malloc(frame_size / 20);

	framebuffer = calloc(1, frame_size);
	if (display == NULL || draw_buffer == NULL || framebuffer == NULL) {
		return EXIT_FAILURE;
	}
	lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
	lv_display_set_buffers(display, draw_buffer, NULL, frame_size / 20,
			       LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display, flush);
	dashboard_create();
	for (int shot = 0; shot < 5;) {
		lv_timer_handler();
		uint32_t elapsed = ticks();

		if (elapsed >= capture_at[shot]) {
			save_frame(argv[1], shot + 1, elapsed);
			shot++;
		}
		struct timespec pause = {.tv_nsec = 10000000};

		nanosleep(&pause, NULL);
	}
	lv_display_delete(display);
	lv_deinit();
	free(draw_buffer);
	free(framebuffer);
	return EXIT_SUCCESS;
}
