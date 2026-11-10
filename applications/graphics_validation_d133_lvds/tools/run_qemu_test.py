#!/usr/bin/env python3
"""Run a Zephyr QEMU ztest and stop only this runner after its terminal result.

Use from the workspace root with west available in PATH. The success
marker, not the emulator's post-test SIGTERM status, determines the test result.
"""
import argparse
import os
import pathlib
import selectors
import signal
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument("build_dir")
parser.add_argument("log_file")
parser.add_argument("--timeout", type=float, default=60)
args = parser.parse_args()
process = subprocess.Popen(
    ["west", "build", "-d", args.build_dir, "-t", "run"],
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, start_new_session=True,
    bufsize=0,
)
selector = selectors.DefaultSelector()
selector.register(process.stdout, selectors.EVENT_READ)
output = bytearray()
terminal = None
deadline = time.monotonic() + args.timeout
try:
    with pathlib.Path(args.log_file).open("wb") as log:
        while time.monotonic() < deadline:
            if not selector.select(0.2):
                continue
            block = os.read(process.stdout.fileno(), 65536)
            if not block:
                break
            log.write(block)
            log.flush()
            output.extend(block)
            if b"PROJECT EXECUTION FAILED" in output:
                terminal = False
                break
            if b"PROJECT EXECUTION SUCCESSFUL" in output:
                terminal = True
                break
finally:
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGTERM)
    process.wait()
    selector.close()
print(f"{args.build_dir}: {'PASS' if terminal else 'FAIL/no terminal success'}")
raise SystemExit(0 if terminal else 1)
