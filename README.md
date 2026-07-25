# Xv6 Operating System Labs

> **Operating Systems Laboratory — University of Tehran — Department of Electrical and Computer Engineering**

![Language](https://img.shields.io/badge/language-C-00599C)
![Platform](https://img.shields.io/badge/platform-xv6%2Fx86-555555)
![Status](https://img.shields.io/badge/status-Completed%20with%20Limitations-f5a623)
![License](https://img.shields.io/badge/license-MIT-green)

## Overview

This repository contains a **single cumulative xv6/x86 operating system** developed through four serial laboratory phases. Each phase modifies the same kernel and user-space source tree: Lab 2 builds on Lab 1, Lab 3 builds on Labs 1–2, and Lab 4 builds on Labs 1–3. The final executable system is therefore the integrated `xv6/` tree, not four independent xv6 copies.

The cumulative kernel adds interactive console editing, checked user utilities, system calls and kernel/user sorting, a configurable multi-level scheduler, and multicore synchronization experiments. Original assignment specifications and submitted reports are retained under `docs/labs/` for provenance, but they are not alternative source trees and are not treated as newly reproduced evidence.

The repository is structured for the MRS-RS P03 C/C++ Console and System Project profile. It includes a root task runner, source and distribution build verification, repository regression tests, QEMU smoke automation, a six-cell syscall-counter matrix, CI workflows, explicit third-party notices, and an auditable reconstructed Git history.

## Objectives

- preserve one bootable xv6 system while applying all four laboratories in sequence;
- retain every earlier phase when introducing later kernel features;
- provide checked interfaces and regression programs for each phase;
- separate source verification from historical report claims;
- make build, smoke testing, counter experiments, and cleanup reproducible;
- maintain a reviewable Git history that records all three contributors in every commit.

## Cumulative phase map

| Phase | Cumulative kernel capability | Principal regression command |
| --- | --- | --- |
| Baseline | MIT xv6/x86 teaching kernel | `usertests` |
| Lab 1 | startup identity, interactive console editing, checked `find_sum` | `lab1test` |
| Lab 2 | PRNG/process/sort system calls and matched kernel/user sorting | `lab2test` |
| Lab 3 | three-queue scheduler, aging, deterministic workloads, measured scheduling statistics | `schedverify` |
| Lab 4 | selectable syscall counters, bounded buffer, writer-priority RW lock, FIFO ticket lock | `scounttest`, `pctest`, `rwtest`, `tickettest` |

The dependency direction is fixed:

```text
MIT xv6 baseline
    -> Lab 1 console and utility changes
    -> Lab 2 system-call and sorting changes
    -> Lab 3 scheduling changes
    -> Lab 4 multicore synchronization changes
    -> one cumulative xv6 image
```

## Architecture

The system keeps the original xv6 kernel/user boundary and extends it in-place:

- `console.c` and `kbd.c` implement line editing and keyboard controls in kernel console context;
- `syscall.c`, `sysproc.c`, `sysfile.c`, `user.h`, and `usys.S` expose the cumulative Lab 2–4 ABI;
- `proc.c`, `proc.h`, `trap.c`, `schedstat.h`, and `cpuwork.h` implement scheduling state, quanta, aging, and measurements;
- `sysproc.c` contains the selectable syscall counters and synchronization services;
- user programs in `xv6/*.c` exercise both the original xv6 facilities and every cumulative laboratory feature;
- the root `Makefile` and `scripts/` define the reproducible host-side execution contract.

A detailed component and interface description is available in [`docs/architecture.md`](docs/architecture.md).

## Repository structure

```text
Xv6-Operating-System-Labs/
├── xv6/                              # Single cumulative xv6/x86 kernel and user-space source tree
│   ├── console.c, kbd.c              # Lab 1 interactive console editing and keyboard handling
│   ├── find_sum.c, lab1test.c        # Checked numeric utility and Lab 1 regression program
│   ├── syscall.c, sysproc.c          # Cumulative syscall dispatch, counters, and synchronization services
│   ├── sysfile.c                     # Checked kernel-side file parsing and sorting entry point
│   ├── sort_kernel.c, sort_user.c    # Equivalent end-to-end Lab 2 sorting paths
│   ├── proc.c, proc.h, trap.c        # Lab 3 scheduler state, dispatch, quanta, aging, and statistics
│   ├── cpuwork.h, schedstat.h        # Deterministic workloads and scheduling measurement ABI
│   ├── schedverify.c                 # Automated Lab 3 scheduler regression program
│   ├── scounttest.c                  # Lab 4 syscall-counter correctness and benchmark driver
│   ├── pctest.c, rwtest.c            # Bounded-buffer and reader-writer lock regressions
│   ├── tickettest.c                  # FIFO ticket-lock regression and cancellation checks
│   └── Makefile                      # Kernel, filesystem image, QEMU, dist, and counter-mode build rules
├── docs/
│   ├── labs/lab1/ ... lab4/          # Assignment PDFs/text, archived reports, and phase-specific notes
│   ├── maintenance/                  # Reconstructed-history and GitHub publication guidance
│   ├── standards/                    # Retained MRS repository standard used for this migration
│   ├── architecture.md               # Cumulative kernel architecture and phase dependency map
│   └── verification.md               # Evidence classes, commands, and success criteria
├── scripts/
│   ├── verify_builds.sh              # Clean source, dist, and all counter-mode build verification
│   ├── qemu_smoke.py                 # Boots one image and checks all Lab 1–4 PASS markers
│   ├── run_counter_matrix.sh         # Builds/runs modes 0–2 with one and four CPUs
│   ├── verify_repository.py          # MRS-RS and cumulative-tree invariant checks
│   └── check_attribution.py          # Verifies three-person attribution on every commit
├── tests/                             # Fast host-side repository regression tests
├── .github/workflows/                # Push/PR CI and full release verification matrix
├── Makefile                           # Standard setup, build, run, test, verify, and clean targets
├── NOTICE.md                          # xv6, assignment, report, and standard rights information
├── LICENSE                            # MIT license for xv6 and repository source code
└── README.md                          # Project overview and execution entry point
```

## Getting started

### Prerequisites

The verified build path targets a Debian/Ubuntu x86-64 host with a 32-bit-capable GCC toolchain:

```bash
sudo apt-get update
sudo apt-get install --yes build-essential gcc-multilib make perl python3 git qemu-system-x86
```

A compatible `i386-jos-elf-*` cross-toolchain may be used by setting `TOOLPREFIX`. QEMU is required for guest-level smoke tests but not for static repository checks or image compilation.

### Installation

```bash
git clone https://github.com/mragetsars/Xv6-Operating-System-Labs.git
cd Xv6-Operating-System-Labs
make setup
```

### Build

```bash
make build
```

Successful completion produces `xv6/kernel`, `xv6/fs.img`, and `xv6/xv6.img` locally. These generated artifacts are intentionally ignored by Git.

### Run

```bash
make run CPUS=2
```

At the xv6 shell, run the cumulative regressions:

```text
lab1test
lab2test
schedverify
scounttest 8 10000
pctest
rwtest
tickettest
```

Exit QEMU with `Ctrl-a`, then `x`.

## Verification and testing

Fast structural and regression checks:

```bash
make lint
make test
```

Clean source, reconstructed `dist`, and three syscall-counter-mode builds:

```bash
make verify-build
```

One cumulative guest smoke run:

```bash
make smoke CPUS=2
```

Complete Lab 4 counter matrix—modes `0`, `1`, and `2`, each with `CPUS=1` and `CPUS=4`:

```bash
make counter-matrix
```

The full local non-QEMU verification contract is:

```bash
make verify
```

Commands, evidence classes, expected markers, and current verification scope are documented in [`docs/verification.md`](docs/verification.md).

## Results and demonstration

| Verification item | Provenance | Current status |
| --- | --- | --- |
| Serial baseline, Lab 1, Lab 2, Lab 3, and Lab 4 stage builds | Verified during repository reconstruction | Passed |
| Final cumulative source build | Verified during repository reconstruction | Passed |
| Reconstructed `dist/` build from its own copied source | Verified during repository reconstruction | Passed |
| Counter modes `0`, `1`, and `2` compilation | Verified during repository reconstruction | Passed |
| Repository invariants and host regression tests | Verified and automated | Passed |
| Guest-level Lab 1–4 PASS markers | QEMU-required; automated in CI | Not executed in the reconstruction host because QEMU was unavailable |
| Screenshots and numeric observations in archived reports | Originally reported | Preserved without relabeling as verified |

This table deliberately distinguishes compilation evidence from runtime evidence. A successful build does not, by itself, prove every scheduler or synchronization behavior.

## Limitations and reproducibility

- The code targets the historical x86 xv6-public codebase, not xv6-riscv.
- A 32-bit compiler/linker path is required; host packages and exact binary layout can differ across toolchains.
- QEMU was not installed in the reconstruction environment. Guest smoke and full counter-matrix workflows are defined but must run in GitHub Actions or another QEMU-equipped host.
- Tick-based timing is intentionally coarse. Benchmark conclusions require repeated runs, raw output retention, and controlled CPU topology.
- `SYSCALL_COUNT_MODE=0` is intentionally race-prone and exists only as the unsafe experimental baseline.
- Exact SMP scheduling order is nondeterministic; policy verification should use invariants and `CPUS=1` when a deterministic sequence is required.
- Console completion uses a static command list because filesystem traversal is unsafe in keyboard-interrupt context.
- Assignment specifications and archived reports are retained for educational provenance and have separate rights from the MIT-licensed source code.

## Git history

The history begins with the xv6 baseline and applies every capability serially. Lightweight phase tags identify stable cumulative points:

```text
lab1-complete
lab2-complete
lab3-complete
lab4-complete
release-ready
```

The history was reconstructed from the supplied project snapshots on 25 July 2026. Historical timestamps were not fabricated. Every commit has one primary author and records the other two team members with `Co-authored-by` trailers; primary authorship is distributed evenly. See [`docs/maintenance/history-reconstruction.md`](docs/maintenance/history-reconstruction.md).

## Contributors

- [Meraj Rastegar](https://github.com/mragetsars) — `mragetsars@gmail.com` — `@mragetsars`
- [Meraj PourHosseiny](https://github.com/MerajPoorhosseiny) — `meraj.prhosseiny@ut.ac.ir` — `@MerajPoorhosseiny`
- [Ali Sadeghi](https://github.com/Alisssaaaddd) — `ali.sadeghi.m@ut.ac.ir` — `@Alisssaaaddd`

## License and data rights

The xv6 source and first-party repository code are distributed under the MIT license in [`LICENSE`](LICENSE). The assignment specifications, archived reports, and retained repository-standard document are not automatically relicensed under MIT; their provenance and rights treatment are described in [`NOTICE.md`](NOTICE.md).

This is an educational operating-system project. It is not intended for production deployment or for protecting sensitive workloads.
