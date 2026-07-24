#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
dist_only=0
[[ ${1:-} == --dist-only ]] && dist_only=1
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
copy_source() {
  local destination=$1
  mkdir -p "$destination"
  git -C "$root" archive HEAD | tar -x -C "$destination"
}
build_copy() {
  local name=$1
  local mode=${2:-}
  local src="$work/$name"
  copy_source "$src"
  make -C "$src" clean >/dev/null 2>&1 || true
  if [[ -n $mode ]]; then
    make -C "$src" -j2 "EXTRA_CFLAGS=-DSYSCALL_COUNT_MODE=$mode" fs.img xv6.img >/dev/null
  else
    make -C "$src" -j2 fs.img xv6.img >/dev/null
  fi
  test -s "$src/kernel" && test -s "$src/fs.img" && test -s "$src/xv6.img"
  printf 'BUILD PASS: %s\n' "$name"
}
if ((dist_only == 0)); then
  build_copy source-default
  for mode in 0 1 2; do build_copy "counter-mode-$mode" "$mode"; done
fi
src="$work/dist-source"
copy_source "$src"
make -C "$src" clean >/dev/null 2>&1 || true
make -C "$src" dist >/dev/null
make -C "$src/dist" -j2 fs.img xv6.img >/dev/null
test -s "$src/dist/kernel" && test -s "$src/dist/fs.img" && test -s "$src/dist/xv6.img"
printf 'BUILD PASS: reconstructed dist\n'
