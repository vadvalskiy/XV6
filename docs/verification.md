# Verification contract

## Evidence vocabulary

| Label | Meaning |
| --- | --- |
| Verified | Executed successfully in the stated environment during reconstruction or CI |
| Artifact-derived | Directly inspected from tracked source, build metadata, or generated test definitions |
| Originally reported | Preserved from an earlier report without a new reproduction claim |
| QEMU-required | Automated command exists, but a QEMU-equipped host is required |
| Illustrative | Example output or explanation; not experimental evidence |

## Host prerequisites

```bash
sudo apt-get update
sudo apt-get install --yes build-essential gcc-multilib make perl python3 git qemu-system-x86
make setup
```

Success criteria:

- every required command is found;
- `gcc -m32` compiles a freestanding object;
- QEMU is detected when guest tests are requested.

## Fast checks

```bash
make lint
make test
```

`make lint` succeeds only when:

- all mandatory MRS-RS root files exist;
- the cumulative xv6 source exists at the repository root and no nested copy exists;
- all four phase document sets exist;
- expected cumulative user programs are present in `UPROGS`;
- syscall numbers are unique;
- generated artifacts are not tracked;
- every Git commit records the three contributors;
- primary-author counts are balanced.

`make test` runs Python `unittest` checks for the cumulative tree, phase regression programs, syscall uniqueness, completion coverage, and required feature sources.

## Clean build verification

```bash
make verify-build
```

This command copies the source into temporary clean directories and verifies:

1. the default cumulative source build;
2. `SYSCALL_COUNT_MODE=0` build;
3. `SYSCALL_COUNT_MODE=1` build;
4. `SYSCALL_COUNT_MODE=2` build;
5. `make dist` followed by a build from the reconstructed `dist/` source itself.

Each case must produce nonempty `kernel`, `fs.img`, and `xv6.img` files. Temporary products are deleted after the check.

## Cumulative guest smoke test

```bash
make smoke CPUS=2
```

The automation boots one cumulative image and requires the following markers in order:

```text
LAB1 TEST PASS
LAB2 TEST PASS
LAB3 TEST PASS
LAB4 COUNTER TEST PASS
LAB4 PC TEST PASS
LAB4 RW TEST PASS
LAB4 TICKET TEST PASS
```

A missing marker, boot timeout, QEMU failure, or early guest termination fails the command.

## Counter matrix

```bash
make counter-matrix
```

The full experiment executes:

| Counter mode | `CPUS=1` | `CPUS=4` |
| --- | --- | --- |
| `0` global unlocked | required | required |
| `1` global locked | required | required |
| `2` per-CPU | required | required |

Mode 0 is permitted to lose increments but must never report a negative delta or a value above the expected count. Modes 1 and 2 must match the expected count. Per-CPU mode must also make the sum of per-CPU deltas equal the aggregated total.

Benchmark interpretation requires retaining raw serial logs and repeating each cell under controlled host load. A single pass establishes regression behavior, not a statistically robust performance conclusion.

## Reconstruction evidence

On 25 July 2026, the following were successfully compiled in the reconstruction environment:

- baseline xv6 stage;
- cumulative Lab 1 stage;
- cumulative Lab 2 stage;
- cumulative Lab 3 stage;
- cumulative Lab 4 stage;
- final source tree;
- reconstructed `dist/` tree;
- all three counter modes.

QEMU was unavailable in that environment. Therefore guest runtime markers are configured in `.github/workflows/ci.yml` and `.github/workflows/full-verification.yml` but are not falsely described as locally executed.

## Cleanup

```bash
make clean
git status --short
```

Success means no generated image, object, executable, log, cache, or `dist/` directory remains and the working tree is clean.
