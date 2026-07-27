#!/usr/bin/env python3
"""Boot the cumulative xv6 image and verify all Lab 1-4 PASS markers."""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import selectors
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
XV6 = ROOT / "xv6"
COMMANDS = [
    ("lab1test", "LAB1 TEST PASS"),
    ("lab2test", "LAB2 TEST PASS"),
    ("schedverify", "LAB3 TEST PASS"),
    ("scounttest 4 2000", "LAB4 COUNTER TEST PASS"),
    ("pctest", "LAB4 PC TEST PASS"),
    ("rwtest", "LAB4 RW TEST PASS"),
    ("tickettest", "LAB4 TICKET TEST PASS"),
]

def qemu_available() -> bool:
    return bool(shutil.which("qemu-system-i386") or shutil.which("qemu-system-x86_64"))

def run(cpus: int, timeout: int, extra_cflags: str) -> int:
    cmd = ["make", "-C", str(XV6), f"CPUS={cpus}"]
    if extra_cflags:
        cmd.append(f"EXTRA_CFLAGS={extra_cflags}")
    cmd.append("qemu-nox")
    env = os.environ.copy(); env.setdefault("TERM", "dumb")
    process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, env=env, bufsize=0)
    assert process.stdin is not None and process.stdout is not None
    selector = selectors.DefaultSelector(); selector.register(process.stdout, selectors.EVENT_READ)
    deadline = time.monotonic() + timeout
    buffer = bytearray(); booted = False; index = 0; sent = False
    try:
        while time.monotonic() < deadline and process.poll() is None:
            for key, _ in selector.select(0.2):
                chunk = os.read(key.fileobj.fileno(), 4096)
                if chunk:
                    buffer.extend(chunk); sys.stdout.buffer.write(chunk); sys.stdout.buffer.flush()
            text = buffer.decode("utf-8", errors="replace")
            if not booted and "init: starting sh" in text:
                booted = True; time.sleep(0.25)
            if booted and index < len(COMMANDS):
                command, marker = COMMANDS[index]
                if not sent:
                    process.stdin.write((command + "\n").encode()); process.stdin.flush(); sent = True
                if marker in text:
                    index += 1; sent = False; buffer.clear()
            if index == len(COMMANDS):
                process.stdin.write(b"\x01x"); process.stdin.flush()
                try: process.wait(timeout=10)
                except subprocess.TimeoutExpired: process.terminate()
                print(f"\nCUMULATIVE QEMU SMOKE PASS: CPUS={cpus}")
                return 0
    finally:
        selector.close()
        if process.poll() is None:
            process.terminate()
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired: process.kill()
        subprocess.run(["make", "-C", str(XV6), "clean"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    missing = COMMANDS[index][1] if index < len(COMMANDS) else "boot prompt"
    print(f"QEMU SMOKE FAIL: missing {missing}", file=sys.stderr)
    return 1

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cpus", type=int, default=2)
    parser.add_argument("--timeout", type=int, default=420)
    parser.add_argument("--extra-cflags", default="")
    parser.add_argument("--skip-missing-qemu", action="store_true")
    args = parser.parse_args()
    if not 1 <= args.cpus <= 8: parser.error("--cpus must be in 1..8")
    if not qemu_available():
        message = "qemu-system-i386/qemu-system-x86_64 is not installed"
        if args.skip_missing_qemu: print(f"SKIP: {message}"); return 0
        print(f"ERROR: {message}", file=sys.stderr); return 2
    return run(args.cpus, args.timeout, args.extra_cflags)

if __name__ == "__main__": raise SystemExit(main())
