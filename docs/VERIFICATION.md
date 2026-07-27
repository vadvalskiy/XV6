# Verification contract

## Evidence vocabulary

| Label | Meaning |
| --- | --- |
| Verified | Executed successfully in the stated environment or CI |
| Artifact-derived | Established directly from tracked source, build rules, or test definitions |
| Originally reported | Preserved from a submitted report without a new reproduction claim |
| QEMU-required | An automated command exists, but a QEMU-equipped host is required |
| Illustrative | Example output or explanation, not experimental evidence |

## Host prerequisites

```bash
sudo apt-get update
sudo apt-get install --yes --no-install-recommends \
  build-essential gcc-multilib make perl python3 git qemu-system-x86
make setup
```

Success requires the build commands, a working 32-bit compilation path, and QEMU when guest execution is requested.

## Fast checks

```bash
make lint
make test
```

`make lint` verifies:

- required public repository files and documentation paths;
- one cumulative `xv6/` source tree and no per-Lab kernel copies;
- four phase summaries, four assignment PDF/text pairs, and four report Markdown/PDF pairs;
- expected guest programs in `xv6/Makefile`;
- unique syscall numbers;
- absence of tracked generated build products;

`make test` executes eight host-side `unittest` regressions covering the cumulative tree, complete Lab documentation, guest regression inclusion, syscall uniqueness, console-completion coverage, and required feature sources.

## Clean build verification

```bash
make verify-build
```

The script copies `xv6/` into temporary directories and verifies:

1. default cumulative source build;
2. `SYSCALL_COUNT_MODE=0` build;
3. `SYSCALL_COUNT_MODE=1` build;
4. `SYSCALL_COUNT_MODE=2` build;
5. `make dist` followed by a build from the reconstructed distribution source.

Every case must produce nonempty `kernel`, `fs.img`, and `xv6.img` files. Temporary workspaces are removed automatically, so verification does not pollute the repository root.

## Cumulative guest smoke test

```bash
make smoke CPUS=2
```

One cumulative image is booted and must emit these markers:

```text
LAB1 TEST PASS
LAB2 TEST PASS
LAB3 TEST PASS
LAB4 COUNTER TEST PASS
LAB4 PC TEST PASS
LAB4 RW TEST PASS
LAB4 TICKET TEST PASS
```

A timeout, QEMU failure, missing marker, or early guest termination fails the command.

## Counter matrix

```bash
make counter-matrix
```

| Counter mode | `CPUS=1` | `CPUS=4` |
| --- | --- | --- |
| `0` global unlocked | required | required |
| `1` global locked | required | required |
| `2` per-CPU | required | required |

Mode `0` may lose increments but must not produce impossible negative or excessive deltas. Modes `1` and `2` must match expected totals. Per-CPU deltas must sum to the aggregated value.

Performance interpretation requires repeated executions, controlled host load, and retained serial logs. A single regression pass is not a statistically robust benchmark.

## Report provenance

The Markdown and PDF reports under `docs/reports/` are complete submitted artifacts. Their descriptions, screenshots, and historical numerical outputs are labeled **Originally reported** unless the corresponding command is rerun and its raw evidence is retained.

The assignment documents under `docs/assignments/` define requirements and are not verification evidence.

## Recorded verification status

Before publication, the repository structure tests, source-default build, all three syscall-counter-mode builds, reconstructed `dist/` build, cumulative QEMU smoke test, and six-cell counter/QEMU matrix completed successfully. The commands above remain the reproducible source of truth for future changes.

## Cleanup

```bash
make clean
git status --short
```

Success means no generated image, object, executable, log, cache, or `dist/` directory remains and the working tree is clean.
