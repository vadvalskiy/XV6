# Xv6 Operating System Labs

> **Operating Systems Laboratory — University of Tehran — Department of Electrical and Computer Engineering**

![Language](https://img.shields.io/badge/language-C-00599C)
![Platform](https://img.shields.io/badge/platform-xv6%2Fx86-555555)
![Status](https://img.shields.io/badge/status-Completed%20with%20Limitations-f5a623)
[![CI](https://github.com/mragetsars/Xv6-Operating-System-Labs/actions/workflows/ci.yml/badge.svg)](https://github.com/mragetsars/Xv6-Operating-System-Labs/actions/workflows/ci.yml)
![License](https://img.shields.io/badge/license-MIT-green)

## Overview

This repository contains one **cumulative xv6/x86 operating system** developed through four serial laboratory projects. Lab 2 extends Lab 1, Lab 3 extends Labs 1–2, and Lab 4 extends Labs 1–3. The final implementation is therefore the single `xv6/` source tree rather than four independent kernel copies.

The cumulative system adds interactive console editing, checked user utilities, new system calls, matched kernel/user sorting paths, a configurable multilevel scheduler, and multicore synchronization experiments. Complete Persian reports and the original assignment specifications are retained under `docs/` alongside the technical documentation.

The upstream xv6/x86 project is historical and is no longer maintained by MIT in favor of xv6-riscv. This project intentionally remains on xv6/x86 because the course specifications and implementations target that codebase.

## Objectives

- preserve one bootable xv6 system while applying all four laboratories in dependency order;
- retain earlier behavior when introducing later kernel changes;
- expose each phase through reviewable source changes, regression programs, reports, and milestone tags;
- distinguish verified builds and tests from observations preserved in submitted reports;
- provide reproducible build, run, verification, and cleanup commands.

## Laboratory phases

| Phase | Cumulative capability | Primary regression | Documentation |
| --- | --- | --- | --- |
| Lab 1 | boot-path study, console editing, and checked `find_sum` | `lab1test` | [Summary](docs/labs/lab-01.md) · [Report](docs/reports/lab-01/) · [Assignment](docs/assignments/lab-01/) |
| Lab 2 | pseudo-random, process-information, and sorting system calls | `lab2test` | [Summary](docs/labs/lab-02.md) · [Report](docs/reports/lab-02/) · [Assignment](docs/assignments/lab-02/) |
| Lab 3 | RR/SJF/FCFS queues, WRR selection, accounting, and aging | `schedverify` | [Summary](docs/labs/lab-03.md) · [Report](docs/reports/lab-03/) · [Assignment](docs/assignments/lab-03/) |
| Lab 4 | syscall counters, bounded buffer, RW lock, and ticket lock | `scounttest`, `pctest`, `rwtest`, `tickettest` | [Summary](docs/labs/lab-04.md) · [Report](docs/reports/lab-04/) · [Assignment](docs/assignments/lab-04/) |

```text
MIT xv6/x86 baseline
    -> Lab 1 console and utility changes
    -> Lab 2 system-call and sorting changes
    -> Lab 3 scheduling changes
    -> Lab 4 synchronization changes
    -> one cumulative kernel and filesystem image
```

## Architecture

The project keeps the original xv6 kernel/user boundary and extends it in place:

- `xv6/console.c` and `xv6/kbd.c` implement line editing and keyboard controls in kernel console context;
- `xv6/syscall.c`, `xv6/sysproc.c`, `xv6/sysfile.c`, `xv6/user.h`, and `xv6/usys.S` expose the cumulative Lab 2–4 ABI;
- `xv6/proc.c`, `xv6/proc.h`, `xv6/trap.c`, `xv6/schedstat.h`, and `xv6/cpuwork.h` implement scheduling state, quanta, aging, and measurements;
- `xv6/sysproc.c` contains the bounded-buffer, reader-writer-lock, and ticket-lock services;
- guest regression programs are compiled into the single xv6 filesystem image;
- the root `Makefile`, `scripts/`, `tests/`, and `.github/workflows/` define the host-side verification workflow.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the component and interface map.

## Repository structure

```text
Xv6-Operating-System-Labs/
├── xv6/                                # Cumulative xv6/x86 kernel and user-space source
│   ├── console.c, kbd.c                # Lab 1 console editor and keyboard path
│   ├── find_sum.c, lab1test.c          # Lab 1 utility and regression
│   ├── syscall.c, sysproc.c, sysfile.c # Cumulative system calls and kernel services
│   ├── sort_kernel.c, sort_user.c      # Lab 2 kernel/user sorting comparison
│   ├── proc.c, proc.h, trap.c          # Lab 3 scheduler, accounting, quanta, and aging
│   ├── schedverify.c                   # Lab 3 scheduler regression
│   ├── scounttest.c                    # Lab 4 syscall-counter regression
│   ├── pctest.c, rwtest.c              # Bounded-buffer and RW-lock regressions
│   ├── tickettest.c                    # FIFO ticket-lock regression
│   └── Makefile                        # xv6 image, QEMU, dist, and counter-mode rules
├── docs/
│   ├── labs/                           # Phase summaries and source maps
│   ├── assignments/lab-01/ ... lab-04/ # Original specifications as PDF and searchable text
│   ├── reports/lab-01/ ... lab-04/     # Complete submitted reports as Markdown and PDF
│   ├── ARCHITECTURE.md                 # Cumulative kernel architecture
│   ├── VERIFICATION.md                 # Verification commands and success criteria
│   ├── CHANGELOG.md                    # Notable project changes
│   ├── CONTRIBUTING.md                 # Contribution workflow
│   ├── SECURITY.md                     # Security scope and reporting guidance
│   ├── CODE_OF_CONDUCT.md              # Participation expectations
│   └── NOTICE.md                       # Third-party materials and artifact rights
├── scripts/                             # Dependency, build, QEMU, and repository checks
├── tests/                               # Fast host-side repository regression tests
├── .github/workflows/                   # Push and pull-request CI
├── Makefile                             # Repository task runner
├── LICENSE                              # MIT license for source and first-party tooling
└── README.md                            # Primary project documentation
```

## Getting started

### Prerequisites

The verified host path targets Debian or Ubuntu on x86-64 with 32-bit compilation support:

```bash
sudo apt-get update
sudo apt-get install --yes --no-install-recommends \
  build-essential gcc-multilib make perl python3 git qemu-system-x86
```

A compatible `i386-jos-elf-*` toolchain can be selected through `TOOLPREFIX`. QEMU is required for guest smoke tests but not for host-side checks or image compilation.

### Installation

```bash
git clone https://github.com/mragetsars/Xv6-Operating-System-Labs.git
cd Xv6-Operating-System-Labs
make setup
```

### Build and run

```bash
make build
make run CPUS=2
```

A successful build produces `xv6/kernel`, `xv6/fs.img`, and `xv6/xv6.img`. These generated files are ignored by Git. Exit QEMU with `Ctrl-a`, then `x`.

## Verification and testing

```bash
make lint                 # Repository structure and source invariants
make test                 # Fast host-side regression tests
make verify-build         # Default, counter-mode, and reconstructed-dist builds
make smoke CPUS=2         # Cumulative Lab 1–4 guest smoke test
make counter-matrix       # Counter modes 0–2 with CPUS=1 and CPUS=4
make clean                # Remove generated build products
```

Detailed success criteria are defined in [`docs/VERIFICATION.md`](docs/VERIFICATION.md).

## Verified results

| Verification item | Status |
| --- | --- |
| Repository structure and eight host regression tests | Passed |
| Default cumulative source build | Passed |
| Counter modes `0`, `1`, and `2` compilation | Passed |
| Reconstructed `dist/` build | Passed |
| Cumulative QEMU smoke test with `CPUS=2` | Passed |
| Six-cell counter/QEMU matrix with `CPUS=1` and `CPUS=4` | Passed |

A successful regression pass establishes the tested invariants, not general performance, fairness, security, or production readiness. Numerical observations preserved in reports remain historical unless reproduced under a controlled environment.

## Limitations and reproducibility

- The project targets the historical `xv6-public` x86 codebase, not `xv6-riscv`.
- A 32-bit compiler and linker path is required; exact binaries can differ across toolchain versions.
- Tick-based timings are coarse and should not be treated as high-resolution benchmarks.
- `SYSCALL_COUNT_MODE=0` is intentionally race-prone and exists as an unsafe experimental baseline.
- Exact SMP scheduling order is nondeterministic; policy tests should use invariants and `CPUS=1` where deterministic ordering is required.
- Static console completion avoids filesystem traversal from keyboard-interrupt context.
- Assignment specifications and reports have rights distinct from the MIT-licensed source; see [`docs/NOTICE.md`](docs/NOTICE.md).
- This educational kernel is not production-ready and must not be used to protect sensitive workloads.

## Milestones

The following tags identify cumulative phase boundaries:

```text
lab1-complete
lab2-complete
lab3-complete
lab4-complete
release-ready
```

The public Git history was reconstructed from archived laboratory submissions before publication. Its commit sequence documents the migration and integration process rather than the original submission timestamps.

## Contributors

- [Meraj Rastegar](https://github.com/mragetsars)
- [Meraj PourHosseiny](https://github.com/MerajPoorhosseiny)
- [Ali Sadeghi](https://github.com/Alisssaaaddd)

## License and artifact rights

The xv6 source and first-party repository tooling are distributed under the MIT license in [`LICENSE`](LICENSE). Assignment specifications and submitted reports retain separate rights and provenance described in [`docs/NOTICE.md`](docs/NOTICE.md).
