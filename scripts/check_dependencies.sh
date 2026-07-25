#!/usr/bin/env bash
set -euo pipefail
required=(gcc ld objdump objcopy make perl python3 git)
missing=()
for command in "${required[@]}"; do
  command -v "$command" >/dev/null 2>&1 || missing+=("$command")
done
if ((${#missing[@]})); then
  printf 'Missing required commands: %s\n' "${missing[*]}" >&2
  exit 1
fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
printf 'int main(void){return 0;}\n' >"$work/check.c"
if ! gcc -m32 -ffreestanding -fno-pie -c "$work/check.c" -o "$work/check.o" >/dev/null 2>&1; then
  echo 'The host compiler cannot emit 32-bit objects with -m32.' >&2
  exit 1
fi
printf 'Required build dependencies: PASS\n'
if command -v qemu-system-i386 >/dev/null 2>&1 || command -v qemu-system-x86_64 >/dev/null 2>&1; then
  printf 'QEMU runtime dependency: PASS\n'
else
  printf 'QEMU runtime dependency: OPTIONAL/MISSING (build verification remains available)\n'
fi
