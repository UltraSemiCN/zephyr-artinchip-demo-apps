/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/display/artinchip_de.h>
#include <zephyr/drivers/misc/artinchip_ge.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#define SIDE 16U
#define PIXELS (SIDE * SIDE)
#define GE_TIMEOUT_MS 250U
static uint32_t source[PIXELS] __aligned(64);
static uint32_t destination[PIXELS] __aligned(64);
static const struct device *const ge = DEVICE_DT_GET(DT_NODELABEL(ge));
static const struct device *const de = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static K_MUTEX_DEFINE(validation_lock);
static bool buffers_quarantined;

BUILD_ASSERT(DT_PROP(DT_CHOSEN(zephyr_display), framebuffer_address) +
	DT_PROP(DT_CHOSEN(zephyr_display), framebuffer_size) <=
	DT_REG_ADDR(DT_CHOSEN(zephyr_sram)), "Framebuffer overlaps application RAM");
BUILD_ASSERT(sizeof(source) % ARTINCHIP_GE_BUFFER_ALIGNMENT == 0);

static struct artinchip_ge_job job_default(void)
{
	return (struct artinchip_ge_job) {
		.op = ARTINCHIP_GE_BLIT,
		.src = {.buffer = source, .buffer_size = sizeof(source), .width = SIDE,
			.height = SIDE, .stride = SIDE * 4, .format = ARTINCHIP_GE_ARGB8888},
		.dst = {.buffer = destination, .buffer_size = sizeof(destination), .width = SIDE,
			.height = SIDE, .stride = SIDE * 4, .format = ARTINCHIP_GE_ARGB8888},
		.src_rect = {.width = SIDE, .height = SIDE},
		.dst_rect = {.width = SIDE, .height = SIDE},
		.global_alpha = 255,
	};
}

static uint16_t rgb565(uint32_t argb)
{
	return ((argb >> 8) & 0xf800U) | ((argb >> 5) & 0x07e0U) | ((argb >> 3) & 0x001fU);
}

static int submit(struct artinchip_ge_job *job)
{
	int ret = artinchip_ge_submit(ge, job, GE_TIMEOUT_MS);

	if (ret != 0) {
		printk("GE operation failed: %d; destination is undefined\n", ret);
		if (ret == -EIO) {
			/* Static buffers survive recovery. Never reuse until recovery succeeds. */
			int recovery = artinchip_ge_reset(ge);

			buffers_quarantined = recovery != 0;
			printk("GE recovery: %d\n", recovery);
		}
	}
	return ret;
}

static int compare32(uint32_t expected, bool alpha)
{
	uint32_t mask = alpha ? UINT32_MAX : 0x00ffffffU;

	for (size_t i = 0; i < PIXELS; i++) {
		if ((destination[i] & mask) != (expected & mask)) {
			printk("pixel %u: %08x expected %08x mask %08x\n",
				(unsigned int)i, destination[i], expected, mask);
			return -EIO;
		}
	}
	return 0;
}

static int fill32(void)
{
	struct artinchip_ge_job job = job_default();

	job.op = ARTINCHIP_GE_FILL;
	job.color = 0xff2468acU;
	int ret = submit(&job);
	return ret == 0 ? compare32(job.color, true) : ret;
}

static int fill565(void)
{
	struct artinchip_ge_job job = job_default();

	job.op = ARTINCHIP_GE_FILL; job.color = 0xffe08020U;
	job.dst.format = ARTINCHIP_GE_RGB565; job.dst.stride = SIDE * 2;
	int ret = submit(&job);

	if (ret != 0) {
		return ret;
	}
	for (size_t i = 0; i < PIXELS; i++) {
		if (((uint16_t *)destination)[i] != rgb565(job.color)) {
			return -EIO;
		}
	}
	return 0;
}

static int blit_gradient(void)
{
	struct artinchip_ge_job job = job_default();

	for (size_t i = 0; i < PIXELS; i++) {
		source[i] = 0xff000000U | ((i * 13U & 255U) << 16) |
			((i * 7U & 255U) << 8) | (i & 255U);
	}
	int ret = submit(&job);
	return ret == 0 && memcmp(source, destination, sizeof(source)) != 0 ? -EIO : ret;
}

static int conversion565(void)
{
	struct artinchip_ge_job job = job_default();

	job.dst.format = ARTINCHIP_GE_RGB565; job.dst.stride = SIDE * 2;
	for (size_t i = 0; i < PIXELS; i++) {
		source[i] = 0xff000000U | (i * 65793U);
	}
	int ret = submit(&job);

	if (ret != 0) {
		return ret;
	}
	for (size_t i = 0; i < PIXELS; i++) {
		if (((uint16_t *)destination)[i] != rgb565(source[i])) {
			return -EIO;
		}
	}
	return 0;
}

static int scale_solid(void)
{
	struct artinchip_ge_job job = job_default();

	job.src_rect.width = 8; job.src_rect.height = 8;
	for (size_t i = 0; i < PIXELS; i++) {
		source[i] = 0xff4080c0U;
	}
	int ret = submit(&job);
	return ret == 0 ? compare32(0xff4080c0U, true) : ret;
}

static int blend(uint8_t alpha)
{
	struct artinchip_ge_job job = job_default();

	job.op = ARTINCHIP_GE_BLEND; job.global_alpha = alpha;
	for (size_t i = 0; i < PIXELS; i++) {
		source[i] = 0xffff0000U; destination[i] = 0xff0000ffU;
	}
	int ret = submit(&job);

	if (ret != 0) {
		return ret;
	}
	/* Hardware may round alpha multiplication by one least-significant bit. */
	for (size_t i = 0; i < PIXELS; i++) {
		int r = (destination[i] >> 16) & 255;
		int g = (destination[i] >> 8) & 255;
		int b = destination[i] & 255;

		if (r < MAX(0, alpha - 1) || r > MIN(255, alpha + 1) || g != 0 ||
		    b < MAX(0, 254 - alpha) || b > MIN(255, 256 - alpha)) {
			return -EIO;
		}
	}
	return 0;
}
static int blend_zero(void)
{
	return blend(0);
}
static int blend_half(void)
{
	return blend(128);
}
static int blend_full(void)
{
	return blend(255);
}

static int transform(enum artinchip_ge_rotation rotation, bool horizontal, bool vertical)
{
	struct artinchip_ge_job job = job_default();

	job.rotation = rotation;
	job.flip_horizontal = horizontal;
	job.flip_vertical = vertical;
	for (unsigned int i = 0; i < PIXELS; i++) {
		source[i] = 0xff000000U | i * 65793U;
	}
	int ret = submit(&job);

	if (ret != 0) {
		return ret;
	}
	for (unsigned int y = 0; y < SIDE; y++) {
		for (unsigned int x = 0; x < SIDE; x++) {
			unsigned int sx = x, sy = y;

			switch (rotation) {
			case ARTINCHIP_GE_ROTATE_90:
				sx = y;
				sy = SIDE - 1U - x;
				break;
			case ARTINCHIP_GE_ROTATE_180:
				sx = SIDE - 1U - x;
				sy = SIDE - 1U - y;
				break;
			case ARTINCHIP_GE_ROTATE_270:
				sx = SIDE - 1U - y;
				sy = x;
				break;
			default:
				break;
			}
			if (horizontal) {
				sx = SIDE - 1U - sx;
			}
			if (vertical) {
				sy = SIDE - 1U - sy;
			}
			if (destination[y * SIDE + x] != source[sy * SIDE + sx]) {
				return -EIO;
			}
		}
	}
	return 0;
}
static int rotate90(void)
{
	return transform(ARTINCHIP_GE_ROTATE_90, false, false);
}
static int rotate180(void)
{
	return transform(ARTINCHIP_GE_ROTATE_180, false, false);
}
static int rotate270(void)
{
	return transform(ARTINCHIP_GE_ROTATE_270, false, false);
}
static int flip_horizontal(void)
{
	return transform(ARTINCHIP_GE_ROTATE_0, true, false);
}
static int flip_vertical(void)
{
	return transform(ARTINCHIP_GE_ROTATE_0, false, true);
}

static int gradient(bool vertical)
{
	struct artinchip_ge_job job = job_default();

	job.op = ARTINCHIP_GE_FILL;
	job.gradient = vertical ? ARTINCHIP_GE_GRADIENT_VERTICAL : ARTINCHIP_GE_GRADIENT_HORIZONTAL;
	job.color = 0xff000000U; job.end_color = 0xffffffffU;
	int ret = submit(&job);

	if (ret != 0) {
		return ret;
	}
	for (unsigned int y = 0; y < SIDE; y++) {
		for (unsigned int x = 0; x < SIDE; x++) {
			uint32_t expected = 0xff000000U | (vertical ? y : x) * 17U * 65793U;

			if (destination[y * SIDE + x] != expected) {
				return -EIO;
			}
		}
	}
	return 0;
}
static int gradient_horizontal(void)
{
	return gradient(false);
}
static int gradient_vertical(void)
{
	return gradient(true);
}

static int grayscale(void)
{
	struct artinchip_ge_job job = job_default();

	job.src.format = ARTINCHIP_GE_YUV400; job.src.stride = SIDE;
	job.color_space = ARTINCHIP_GE_BT601_FULL;
	memset(source, 128, sizeof(source));
	int ret = submit(&job);

	if (ret != 0) {
		return ret;
	}
	for (unsigned int i = 0; i < PIXELS; i++) {
		for (unsigned int shift = 0; shift < 24; shift += 8) {
			unsigned int value = (destination[i] >> shift) & 255;

			if (value < 127 || value > 129) {
				return -EIO;
			}
		}
	}
	return 0;
}

static int color_key(void)
{
	struct artinchip_ge_job job = job_default();

	job.color_key_enable = true; job.color_key = 0xff0000U;
	for (unsigned int i = 0; i < PIXELS; i++) {
		source[i] = i % 2U ? 0xff00ff00U : 0xffff0000U;
		destination[i] = 0xff0000ffU;
	}
	int ret = submit(&job);

	if (ret != 0) {
		return ret;
	}
	for (unsigned int i = 0; i < PIXELS; i++) {
		uint32_t expected = i % 2U ? 0x0000ff00U : 0x000000ffU;

		if ((destination[i] & 0xffffffU) != expected) {
			return -EIO;
		}
	}
	return 0;
}

static int batch_dependency(void)
{
	struct artinchip_ge_job jobs[2] = {job_default(), job_default()};

	jobs[0].op = ARTINCHIP_GE_FILL;
	jobs[0].dst = jobs[0].src;
	jobs[0].color = 0xff27a56cU;
	int ret = artinchip_ge_submit_batch(ge, jobs, 2, GE_TIMEOUT_MS);

	if (ret == -EIO) {
		buffers_quarantined = artinchip_ge_reset(ge) != 0;
	}
	return ret == 0 ? compare32(jobs[0].color, true) : ret;
}

static int in_place_copy(unsigned int direction)
{
	struct artinchip_ge_job job = job_default();
	uint32_t expected[PIXELS];

	job.src = job.dst;
	job.in_place = true;
	job.src_rect.width = 12; job.src_rect.height = 12;
	job.dst_rect.width = 12; job.dst_rect.height = 12;
	job.src_rect.x = direction & 1U ? 2 : 0;
	job.dst_rect.x = direction & 1U ? 0 : 2;
	job.src_rect.y = direction & 2U ? 2 : 0;
	job.dst_rect.y = direction & 2U ? 0 : 2;
	for (size_t i = 0; i < PIXELS; i++) {
		destination[i] = 0xff000000U | (i * 0x00010101U);
		expected[i] = destination[i];
	}
	/* Golden reads the original image, never already-overwritten neighbors. */
	for (size_t y = 0; y < 12; y++) {
		for (size_t x = 0; x < 12; x++) {
			expected[(y + job.dst_rect.y) * SIDE + x + job.dst_rect.x] =
				destination[(y + job.src_rect.y) * SIDE + x + job.src_rect.x];
		}
	}
	int ret = submit(&job);
	return ret == 0 && memcmp(expected, destination, sizeof(expected)) != 0 ? -EIO : ret;
}
static int inplace_down_right(void)
{
	return in_place_copy(0);
}
static int inplace_down_left(void)
{
	return in_place_copy(1);
}
static int inplace_up_right(void)
{
	return in_place_copy(2);
}
static int inplace_up_left(void)
{
	return in_place_copy(3);
}

static int de_copy_readback(void)
{
	uint16_t saved[8], input[8] = {0xf800, 0x07e0, 0x001f, 0xffff, 0, 0xaaaa, 0x5555, 0x1234};
	uint16_t output[8] = {0};
	struct display_buffer_descriptor desc = {.width = 4, .height = 2, .pitch = 4,
		.buf_size = sizeof(input)};
	int ret = display_read(de, 12, 12, &desc, saved);

	if (ret != 0) {
		return ret;
	}
	ret = display_write(de, 12, 12, &desc, input);
	if (ret == 0) {
		ret = display_read(de, 12, 12, &desc, output);
	}
	int restore = display_write(de, 12, 12, &desc, saved);

	if (ret == 0 && restore != 0) {
		ret = restore;
	}
	if (ret == 0 && memcmp(input, output, sizeof(input)) != 0) {
		ret = -EIO;
	}
	return ret;
}
static int de_vsync(void)
{
	return artinchip_de_wait_vsync(de, 100);
}

static int run_validation(void)
{
	static const struct {
		const char *name;
		int (*run)(void);
	} cases[] = {
		{"GE fill ARGB8888", fill32}, {"GE fill RGB565", fill565},
		{"GE gradient blit", blit_gradient}, {"GE ARGB8888 to RGB565", conversion565},
		{"GE 2x solid scale", scale_solid}, {"GE alpha 0", blend_zero},
		{"GE alpha 128", blend_half}, {"GE alpha 255", blend_full},
		{"GE rotate 90", rotate90}, {"GE rotate 180", rotate180},
		{"GE rotate 270", rotate270}, {"GE flip H", flip_horizontal},
		{"GE flip V", flip_vertical}, {"GE gradient H", gradient_horizontal},
		{"GE gradient V", gradient_vertical}, {"GE YUV400 full-range", grayscale},
		{"GE source color key", color_key},
		{"GE hardware CMDQ fill-copy", batch_dependency},
		{"GE in-place down/right", inplace_down_right},
		{"GE in-place down/left", inplace_down_left},
		{"GE in-place up/right", inplace_up_right},
		{"GE in-place up/left", inplace_up_left},
		{"DE CPU copy/readback", de_copy_readback}, {"DE fresh VBLANK IRQ", de_vsync},
	};
	if (!device_is_ready(ge) || !device_is_ready(de)) {
		return -ENODEV;
	}
	if (k_mutex_lock(&validation_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}
	if (buffers_quarantined) {
		printk("Buffers quarantined after failed recovery; reboot before retry.\n");
		k_mutex_unlock(&validation_lock);
		return -EIO;
	}
	printk("GE version 0x%08x; CPU golden comparisons, not panel verification\n",
		artinchip_ge_get_version(ge));
	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		int ret = cases[i].run();
		const char *result = ret == 0 ? "PASS" : "FAIL";

		printk("%s: %s (%d)\n", cases[i].name, result, ret);
		if (ret != 0) {
			/* Stop before accessing quarantined buffers after a failed reset. */
			k_mutex_unlock(&validation_lock);
			return ret;
		}
	}
	printk("%u/%u on-device checks PASS; panel visual NOT_VERIFIED\n",
		(unsigned int)ARRAY_SIZE(cases), (unsigned int)ARRAY_SIZE(cases));
	printk("TE/single-frame SKIP: this LVDS continuous-scanout profile has no TE input\n");
	k_mutex_unlock(&validation_lock);
	return 0;
}

static int cmd_verify(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh); ARG_UNUSED(argc); ARG_UNUSED(argv);
	return run_validation();
}
static int cmd_panel(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	struct artinchip_de_ui_rect full = {.width = 1024, .height = 600, .stride = 2048};
	int ret = display_blanking_on(de);

	if (ret == 0) {
		ret = artinchip_de_set_video(de, NULL);
	}
	if (ret == 0) {
		ret = artinchip_de_set_ui_layout(de, &full, 1);
	}
	if (ret == 0) {
		ret = artinchip_de_test_pattern(de);
	}
	if (ret == 0) {
		ret = display_blanking_off(de);
	}
	shell_print(sh, "Color bars %s (%d). Visual check required: white/yellow/cyan/green/"
		"magenta/red/blue/black; full 1024x600, no flicker/shift.",
		ret == 0 ? "submitted" : "failed", ret);
	return ret;
}
static int cmd_video(const struct shell *sh, size_t argc, char **argv)
{
	struct artinchip_de_framebuffer frame;
	struct artinchip_de_video video = {
		.format = ARTINCHIP_DE_VIDEO_NV12, .width = 512, .height = 300,
		.crop = {.width = 512, .height = 300},
		.output = {.width = 1024, .height = 600},
		.planes = {{.offset = 0, .stride = 512}, {.offset = 153600, .stride = 512}},
		.color_space = 0,
	};
	ARG_UNUSED(argc); ARG_UNUSED(argv);
	int ret = display_blanking_on(de);

	if (ret == 0) {
		ret = artinchip_de_acquire_framebuffer(de, &frame, 100);
	}
	if (ret == 0) {
		/* Limited-range neutral chroma: eight ascending grayscale bars. */
		if (frame.size < 230400U) {
			ret = -ENOMEM;
		} else {
			uint8_t *pixels = frame.buf;

			for (size_t y = 0; y < 300U; y++) {
				for (size_t x = 0; x < 512U; x++) {
					pixels[y * 512U + x] = 16U + (x / 64U) * 219U / 7U;
				}
			}
			memset(pixels + 153600U, 128, 76800U);
		}
		int release = artinchip_de_release_framebuffer(de);

		if (ret == 0) {
			ret = release;
		}
	}
	if (ret == 0) {
		ret = artinchip_de_set_ui_layout(de, NULL, 0);
	}
	if (ret == 0) {
		ret = artinchip_de_set_video(de, &video);
	}
	if (ret == 0) {
		ret = display_blanking_off(de);
	}
	shell_print(sh, "NV12 512x300 -> 1024x600: %s (%d). Manual check only: "
		"eight grayscale bars black to white. DE CSC pixels NOT_VERIFIED.",
		ret == 0 ? "submitted" : "failed", ret);
	return ret;
}

static int cmd_stats(const struct shell *sh, size_t argc, char **argv)
{
	struct artinchip_de_stats stats;

	ARG_UNUSED(argc); ARG_UNUSED(argv);
	artinchip_de_get_stats(de, &stats);
	shell_print(sh, "vsync=%u underflow=%u; counters do not prove panel pixels",
		stats.vsync_count, stats.underflow_count);
	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(graphics_commands,
	SHELL_CMD(verify, NULL, "Run GE CPU-golden and DE checks", cmd_verify),
	SHELL_CMD(panel, NULL, "Show color bars for manual LVDS check", cmd_panel),
	SHELL_CMD(video, NULL, "Show scaled NV12 for manual LVDS check", cmd_video),
	SHELL_CMD(stats, NULL, "Read DE IRQ counters", cmd_stats),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(gfx, &graphics_commands, "D133 GE/DE validation", NULL);

int main(void)
{
	printk("D133 GE/DE validation firmware. No test has run yet.\n");
	printk("Run gfx verify for DMA/readback checks; gfx panel for manual panel inspection.\n");
	return 0;
}
