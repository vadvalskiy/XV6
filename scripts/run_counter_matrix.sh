#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_only=0
[[ ${1:-} == --build-only ]] && build_only=1
for mode in 0 1 2; do
  make -C "$root/xv6" clean >/dev/null 2>&1 || true
  make -C "$root/xv6" -j2 "EXTRA_CFLAGS=-DSYSCALL_COUNT_MODE=$mode" fs.img xv6.img >/dev/null
  printf 'COUNTER BUILD PASS: mode=%d\n' "$mode"
done
make -C "$root/xv6" clean >/dev/null 2>&1 || true
if ((build_only)); then exit 0; fi
for mode in 0 1 2; do
  for cpus in 1 4; do
    python3 "$root/scripts/qemu_smoke.py" --cpus "$cpus" \
      --extra-cflags "-DSYSCALL_COUNT_MODE=$mode"
  done
done
