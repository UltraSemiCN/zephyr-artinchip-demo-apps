# D133 GE / DE validation

Standalone graphics validation for `artinchip_d13x_demo88_nor/d13x`, using
its 1024x600 VESA 24-bit single-link LVDS panel. This application uses the GE
and DE driver APIs directly; it does not enable LVGL hardware acceleration.

## Build

From the workspace root, with dependencies installed:

```sh
. .venv/bin/activate
export ZEPHYR_BASE="$PWD/zephyr-artinchip"
# Set ZEPHYR_SDK_INSTALL_DIR if the SDK is not discovered automatically.
west build --sysbuild -p always -b artinchip_d13x_demo88_nor/d13x \
  zephyr-artinchip-demo-apps/applications/graphics_validation_d133_lvds \
  -d build/graphics_validation_d133_lvds -- \
  -DZEPHYR_EXTRA_MODULES="$PWD/zephyr-artinchip-tools"
python zephyr-artinchip-tools/tests/verify_mcuboot_build.py \
  build/graphics_validation_d133_lvds
```

The image is generated in `build/graphics_validation_d133_lvds/airburn/`.
This uses PBP, MCUboot, signed RAM-load and development signing keys.
The framebuffer occupies `0x40000000..0x4012bfff`; application RAM is
`0x40130000..0x401fffff`. This layout reserves enough framebuffer memory for
full-screen RGB565, not full-screen 32-bit color.

## Board commands

The application starts a serial shell and waits for commands:

| Command | Purpose |
| --- | --- |
| `gfx verify` | Run 24 hardware checks, including GE output versus CPU reference pixels, DE readback and VBLANK IRQ. Stop on failure. |
| `gfx panel` | Show eight vertical color bars; also restore the RGB565 UI after video tests. |
| `gfx video` | Scale NV12 512x300 to 1024x600 for visual grayscale inspection. |
| `gfx stats` | Report VBLANK and underflow counters. |

Inspect panel colors, orientation, boundaries and stability on hardware.
Fake-MMIO tests and image verification do not prove real DMA or panel behavior.
Hardware acceptance of these new graphics features is pending.

## API and validation boundaries

- GE submits and bounded command batches are synchronous. Buffers must remain
  exclusively owned until completion or successful recovery after an error.
- GE operates on separate aligned allocations. Copy completed output to DE;
  do not let GE write an actively scanned framebuffer.
- DE framebuffer leases serialize software producers and pause timing; they
  do not prove that outstanding DMA reads have drained. Release from the same
  thread that acquired the lease.
- Advanced DE V1.1 features require `artinchip,de-version = <11>` and blanking
  before configuration. Video planes use offsets in the fixed framebuffer.
- D133 GE V1.1 does not support general multi-plane YUV input. DE video input
  is a separate capability. TE/single-frame and writeback are not implemented.
- Verify cache-enabled DMA, timeout/reset recovery, repeated operations and
  underflow behavior on the board before treating this as hardware validated.

Software tests compile the actual drivers against fake MMIO/cache backends:
`zephyr-artinchip/tests/drivers/artinchip_ge`,
`zephyr-artinchip/tests/drivers/display/artinchip_de` and
`zephyr-artinchip/tests/drivers/clock_control/artinchip`.
They support `native_sim/native/64` and `qemu_riscv32`. After building a test,
`tools/run_qemu_test.py <build-directory> <log-file>` runs it and checks the
terminal ztest result. It also works with the native simulator.
