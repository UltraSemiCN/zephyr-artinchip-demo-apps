#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

if [[ ${1:-} == --help || ${1:-} == -h ]]; then
    echo 'Usage: build_dashboard_720x1280_d125_mipi.sh'
    echo 'Build and verify the UltraSemi D125 720x1280 MIPI Dashboard.'
    echo 'Uses workspace .venv and build/dashboard_720x1280_d125_mipi.'
    echo 'Set ZEPHYR_SDK_INSTALL_DIR to override the workspace SDK setting.'
    exit 0
fi
if (( $# != 0 )); then
    echo 'Unexpected arguments; use --help for usage.' >&2
    exit 2
fi

app_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
workspace=$(cd -- "$app_root/.." && pwd)
app=dashboard_720x1280_d125_mipi
build_dir="$workspace/build/$app"
export ZEPHYR_BASE="$workspace/zephyr-artinchip"

if [[ -d "$workspace/.venv/bin" ]]; then
    export PATH="$workspace/.venv/bin:$PATH"
fi
for tool in west python3 cmake ninja; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing tool: $tool. Install the README prerequisites first." >&2
        exit 1
    fi
done
for path in "$ZEPHYR_BASE" "$workspace/zephyr-artinchip-tools" \
            "$workspace/bootloader/mcuboot" "$workspace/.west"; do
    if [[ ! -d "$path" ]]; then
        echo "Missing workspace directory: $path" >&2
        exit 1
    fi
done
cd -- "$workspace"
if [[ -z ${ZEPHYR_SDK_INSTALL_DIR:-} ]]; then
    sdk_dir=$(west config build.zephyr-sdk 2>/dev/null || true)
    if [[ -n "$sdk_dir" ]]; then
        [[ "$sdk_dir" = /* ]] || sdk_dir="$workspace/$sdk_dir"
        export ZEPHYR_SDK_INSTALL_DIR="$sdk_dir"
    fi
fi
if [[ -n ${ZEPHYR_SDK_INSTALL_DIR:-} && ! -d "$ZEPHYR_SDK_INSTALL_DIR" ]]; then
    echo "SDK directory not found: $ZEPHYR_SDK_INSTALL_DIR" >&2
    exit 1
fi

west build --sysbuild -p always \
    -b ultrasemi_d12p_usbdisp_demo88_nor_5_5/d125ccv \
    "$app_root/applications/$app" -d "$build_dir" -- \
    -DZEPHYR_EXTRA_MODULES="$workspace/zephyr-artinchip-tools"
python3 "$workspace/zephyr-artinchip-tools/tests/verify_mcuboot_build.py" "$build_dir"

shopt -s nullglob
images=("$build_dir/airburn/"*.img)
if (( ${#images[@]} == 0 )); then
    echo "No AirBurn image generated in $build_dir/airburn" >&2
    exit 1
fi
printf '\nAirBurn image(s):\n'
printf '%s\n' "${images[@]}"
sha256sum "${images[@]}"
