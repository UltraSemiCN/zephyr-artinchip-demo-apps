# ArtInChip Zephyr demos

LVGL dashboards and a boot-shell test maintained by UltraSemi.
Hardware drivers live in `zephyr-artinchip`; firmware packaging lives in
`zephyr-artinchip-tools`.

## Dashboards

| Application | Board target | Display |
| --- | --- | --- |
| `dashboard_1024x600_d125_mipi` | `artinchip_d12p_demo88_nor/d125ccr` | 1024×600 MIPI |
| `dashboard_720x1280_d125_mipi` | `ultrasemi_d12p_usbdisp_demo88_nor_5_5/d125ccv` | 720×1280 MIPI |
| `dashboard_1024x600_d133_lvds` | `artinchip_d13x_demo88_nor/d13x` | 1024×600 LVDS |
| `dashboard_1024x600_d213_lvds` | `artinchip_d21x_demo128_nand/d21x` | 1024×600 LVDS |

All dashboards provide animated simulated readings and a UART shell
(115200 baud, 8N1, no flow control). Enter `help` for commands or `aicupg`
to reboot into ROM USB upgrade mode.

All profiles show a centered, chip-specific `Zephyr LVGL Dashboard` title,
four gauges, a chart, progress bars and status lights. Two equal-width
columns center the partner logos and their websites.

## Workspace setup

Prerequisites: Git, Python 3.12+, CMake 3.28+, Ninja, a supported host toolchain
and Zephyr SDK (tested with 1.0.1). Install the host prerequisites listed in the
[Zephyr getting started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).

```sh
mkdir workspace_artinchip_github
cd workspace_artinchip_github
git clone https://github.com/UltraSemiCN/zephyr-artinchip-demo-apps.git
python3 -m venv .venv
. .venv/bin/activate
pip install west
west init -l zephyr-artinchip-demo-apps
west update
pip install -r zephyr-artinchip/scripts/requirements.txt
pip install -r bootloader/mcuboot/scripts/requirements.txt littlefs-python
```

If the SDK is not discovered automatically, set `ZEPHYR_SDK_INSTALL_DIR` to
its installation directory. Build commands follow below.

The application owns `west.yml`. ArtInChip repositories and upstream modules
use pinned revisions. Review dependency updates, rebuild affected profiles,
and repeat board tests before changing pins. For reproducible builds, use
clean checkouts at the manifest revisions.

```sh
west manifest --validate
west list
```

## Build and flash

Run from the workspace root:

```sh
export ZEPHYR_BASE="$PWD/zephyr-artinchip"
app=dashboard_1024x600_d125_mipi
board=artinchip_d12p_demo88_nor/d125ccr
west build --sysbuild -p always -b "$board" \
  "zephyr-artinchip-demo-apps/applications/$app" -d "build/$app" -- \
  -DZEPHYR_EXTRA_MODULES="$PWD/zephyr-artinchip-tools"
python zephyr-artinchip-tools/tests/verify_mcuboot_build.py "build/$app"
```

Select another application/board pair from the table as needed. AirBurn images
are generated under `build/<application>/airburn/`. Builds use MCUboot RAM-load
and development signing keys; use your own keys for production.

Flash the generated `.img` with the ArtInChip AirBurn tool for your board.
The `aicupg` shell command enters ROM USB upgrade mode; it does not upload
firmware itself.

## Boot-shell test

For a minimal D213 NAND boot test without display initialization:

```sh
west build --sysbuild -p always -b artinchip_d21x_demo128_nand/d21x \
  zephyr-artinchip-demo-apps/applications/boot-shell -d build/d213_mcuboot_shell -- \
  -DZEPHYR_EXTRA_MODULES="$PWD/zephyr-artinchip-tools"
python zephyr-artinchip-tools/scripts/mk_image_zephyr.py --build-dir build/d213_mcuboot_shell
```

To test LVDS color bars and a moving marker, add these CMake arguments to
the same pristine build command:

```sh
-Dboot-shell_EXTRA_CONF_FILE=d213-lvds.conf \
-Dboot-shell_EXTRA_DTC_OVERLAY_FILE=d213-lvds.overlay
```

Use a pristine build when switching back to the shell-only test to clear the
extra configuration. The output remains
`build/d213_mcuboot_shell/airburn/artinchip_d21x_demo128_nand_v0.1.0.img`.

## Validation status

Internal hardware results do not automatically validate the GitHub dependency
revisions pinned in `west.yml`.

| Scope | Evidence |
| --- | --- |
| Four baseline dashboards | User-reported board tests passed on 2026-10-12 |
| Latest landscape branding | Internal D125 MIPI build and 19 MCUboot/AirBurn checks passed on 2026-10-16 |
| Latest branding on hardware | Not yet confirmed |
| Public four-profile build at `786c40e` | All built; 19 checks passed per NOR image and 23 for D213 NAND; no new board results |
| New portrait branding | Internal build and 19 MCUboot/AirBurn checks passed; hardware verification pending |

Before a release, build the affected profiles from the public manifest and
verify boot, UART input, animated display, and ROM upgrade entry on hardware.

## Logo assets

`artinchip.png` and `ultrasemi.png` retain the supplied originals. Logo rights
remain with their owners. All four profiles embed the same RGB565A8 C arrays.

Regenerate from the application repository root with Python and Pillow:

```sh
python3 common/dashboard/scripts/logo_to_lvgl.py \
  common/dashboard/assets/artinchip.png common/dashboard/src/assets/artinchip_logo.c \
  --symbol dashboard_artinchip_logo --height 48
python3 common/dashboard/scripts/logo_to_lvgl.py \
  common/dashboard/assets/ultrasemi.png common/dashboard/src/assets/ultrasemi_logo.c \
  --symbol dashboard_ultrasemi_logo --height 48 --alpha-threshold 16
```

The alpha threshold excludes faint speckles when finding the UltraSemi crop
bounds. Colors and alpha inside the bounds are preserved before scaling.
Both logos are 48 pixels high: ArtInChip is 159 pixels wide and UltraSemi is
202 pixels wide (51,984 bytes total). If either exceeds the 204-pixel limit,
reduce both heights together to preserve proportions.

The header content area has two equal columns in both orientations. Logos and their websites
are centered within each column. No runtime PNG decoder or filesystem is needed.

## License

See [LICENSE](LICENSE). Partner logos remain the property of their owners.
