#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Run four real-time LVGL renderers and save five random frames per profile."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
import hashlib
import html
import json
from pathlib import Path
import random
import secrets
import subprocess

from PIL import Image, PngImagePlugin

PROFILES = {
    'dashboard_1024x600_d125_mipi': ('D125 Demo MIPI', (1024, 600)),
    'dashboard_720x1280_d125_mipi': ('D125 UltraSemi MIPI', (720, 1280)),
    'dashboard_1024x600_d133_lvds': ('D133 Demo LVDS', (1024, 600)),
    'dashboard_1024x600_d213_lvds': ('D213 Demo NAND/LVDS', (1024, 600)),
}


def main():
    app = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path,
                        default=app.parent / 'build/dashboard_simulation')
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--seed', type=int, default=secrets.randbits(64))
    args = parser.parse_args()
    build = args.build_dir.resolve()
    output = (args.output_dir or build / 'screenshots' /
              datetime.now().strftime('%Y%m%d-%H%M%S')).resolve()
    output.mkdir(parents=True, exist_ok=False)
    randomizer = random.Random(args.seed)
    times = {name: sorted(randomizer.sample(range(3000, 12001, 250), 5))
             for name in PROFILES}

    def capture(name):
        directory = output / name
        directory.mkdir()
        command = [str(build / name), str(directory), *map(str, times[name])]
        run = subprocess.run(command, text=True, capture_output=True, check=True, timeout=45)
        (directory / 'run.log').write_text(run.stdout + run.stderr)
        frames = sorted(directory.glob('frame_*.ppm'))
        assert len(frames) == 5, (name, len(frames))
        records = []
        hashes = set()
        for ppm in frames:
            png = ppm.with_suffix('.png')
            with Image.open(ppm) as frame:
                assert frame.size == PROFILES[name][1]
                info = PngImagePlugin.PngInfo()
                info.add_text('Description', 'LVGL host framebuffer simulation, not hardware capture')
                info.add_text('Profile', name)
                info.add_text('Seed', str(args.seed))
                frame.save(png, pnginfo=info)
                hashes.add(hashlib.sha256(frame.tobytes()).hexdigest())
            ppm.unlink()
            records.append({'file': png.relative_to(output).as_posix(),
                            'actual_ms': int(png.stem.split('_')[-1][:-2]),
                            'sha256': hashlib.sha256(png.read_bytes()).hexdigest()})
        assert len(hashes) == 5, f'{name}: repeated frames; animation may be stalled'
        return {'profile': name, 'resolution': PROFILES[name][1],
                'requested_ms': times[name], 'frames': records}

    with ThreadPoolExecutor(max_workers=4) as pool:
        results = list(pool.map(capture, PROFILES))
    commit = subprocess.check_output(['git', '-C', str(app), 'rev-parse', 'HEAD'],
                                     text=True).strip()
    report = {'kind': 'LVGL software framebuffer simulation', 'seed': args.seed,
              'application_commit': commit, 'profiles': results,
              'limitations': 'Host timing and memory; no SoC, panel, UART or boot-chain simulation.'}
    (output / 'capture.json').write_text(json.dumps(report, indent=2) + '\n')
    sections = []
    for result in results:
        cards = ''.join(f'<a href="{f["file"]}"><img src="{f["file"]}" '
                        f'alt="{html.escape(result["profile"])}"><span>'
                        f'{f["actual_ms"] / 1000:.3f} s</span></a>' for f in result['frames'])
        sections.append(f'<h2>{html.escape(PROFILES[result["profile"]][0])}</h2>'
                        f'<div class="frames">{cards}</div>')
    (output / 'index.html').write_text('''<!doctype html><meta charset="utf-8">
<title>Dashboard simulation captures</title><style>
body{background:#101923;color:#eee;font:16px sans-serif;margin:24px}
.frames{display:flex;gap:12px;align-items:flex-start}a{flex:1;min-width:0;color:#9de}
img{width:100%;height:auto}span{display:block;margin-top:8px}h2{margin-top:32px}
</style><h1>LVGL Dashboard simulations</h1>
<p>Five randomly timed frames per profile, captured after 3–12 seconds.
Software rendering only; not hardware screenshots. Click an image for full resolution.</p>
''' + '\n'.join(sections))
    print(output)
    print('Saved 20 PNG screenshots; dimensions and distinct animated frames verified.')


if __name__ == '__main__':
    main()
