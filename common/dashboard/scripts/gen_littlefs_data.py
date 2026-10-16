#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 UltraSemi Technology Development Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

"""Build a LittleFS image for the ArtInChip SPI NOR data partition."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source",
        type=Path,
        required=True,
        help="Directory tree to pack into the filesystem",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Output image path (for example zephyr/data.lfs)",
    )
    parser.add_argument("--block-size", type=int, default=65536)
    parser.add_argument("--block-count", type=int, default=16)
    parser.add_argument("--read-size", type=int, default=16)
    parser.add_argument("--prog-size", type=int, default=16)
    parser.add_argument("--lookahead-size", type=int, default=32)
    parser.add_argument("--block-cycles", type=int, default=512)
    return parser.parse_args(argv)


def collect_files(source: Path) -> list[tuple[str, Path]]:
    files: list[tuple[str, Path]] = []

    for path in sorted(source.rglob("*")):
        if not path.is_file():
            continue
        if path.name.startswith("."):
            continue
        rel = path.relative_to(source).as_posix()
        files.append((rel, path))

    return files


def main(argv: list[str] | None = None) -> int:
    try:
        from littlefs import LittleFS
    except ImportError:
        print(
            "littlefs-python is required: pip install littlefs-python",
            file=sys.stderr,
        )
        return 1

    args = parse_args(argv or sys.argv[1:])
    source = args.source.resolve()

    if not source.is_dir():
        print(f"source directory not found: {source}", file=sys.stderr)
        return 1

    files = collect_files(source)
    if not files:
        print(f"no files to pack under {source}", file=sys.stderr)
        return 1

    fs = LittleFS(
        block_size=args.block_size,
        block_count=args.block_count,
        read_size=args.read_size,
        prog_size=args.prog_size,
        lookahead_size=args.lookahead_size,
        block_cycles=args.block_cycles,
    )

    for rel, path in files:
        parent = "/".join(rel.split("/")[:-1])
        if parent:
            parts = parent.split("/")
            so_far = ""
            for part in parts:
                so_far = part if not so_far else so_far + "/" + part
                try:
                    fs.mkdir(so_far)
                except OSError:
                    pass
        with fs.open(rel, "wb") as handle:
            handle.write(path.read_bytes())

    image = fs.context.buffer
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(image)

    names = ", ".join(rel for rel, _ in files)
    print(f"Packed {len(files)} file(s) into {args.output} ({len(image)} bytes): {names}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
