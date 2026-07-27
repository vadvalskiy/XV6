# Changelog

This academic repository does not use artificial semantic versions. Changes are organized by cumulative laboratory phase and by the reconstructed Git history.

## 27 July 2026 — public repository cleanup

### Documentation and privacy

- consolidated reports and project policy files under `docs/`;
- removed the separate Persian README, citation metadata, mailmap, and internal migration records;
- removed public email literals from maintained documentation and verification scripts;
- corrected the counter-matrix CLI argument handling and added a regression test.

## 26 July 2026 — repository layout and artifact correction

### Structure

- moved the single cumulative kernel and user-space tree into `xv6/`;
- restored a clean repository-level task runner and path-aware CI tooling;
- removed migration-only hidden marker files;
- kept xv6 build products out of the repository root.

### Documentation artifacts

- restored all four complete Markdown reports;
- added all four final PDF reports under `docs/reports/`;
- separated assignment specifications into `docs/assignments/`;
- added phase summaries and explicit evidence/provenance links;
- corrected attribution verification so upstream xv6 commits are excluded from coursework author checks.

## 25 July 2026 — cumulative repository reconstruction

### Repository architecture

- replaced parallel per-Lab xv6 copies with one cumulative xv6 source tree;
- preserved assignment specifications and report sources as reconstruction inputs;
- added a focused public repository layout, task runner, notices, and verification documentation;
- reconstructed an atomic, serial Git history without fabricating historical timestamps.

### Lab 1

- retained startup contributor identity output;
- retained interactive console cursor, selection, clipboard, undo, and completion behavior;
- hardened signed/unsigned integer parsing and overflow handling in `find_sum`;
- added the `lab1test` regression program.

### Lab 2

- retained random-number, process-information, and kernel sorting system calls;
- aligned kernel and user sorting to equivalent end-to-end timing scopes;
- hardened integer parsing and shortened PRNG critical sections;
- added the `lab2test` regression program.

### Lab 3

- retained the cumulative three-level scheduler, quanta, weighted selection, and aging;
- replaced wall-clock workloads with deterministic CPU work;
- added runtime, runnable time, dispatch, preemption, response, and turnaround measurements;
- added `waitstats` and the `schedverify` regression program.

### Lab 4

- retained selectable unlocked, locked, and per-CPU syscall counters;
- added nonblocking and blocking bounded-buffer interfaces without `-1` value ambiguity;
- added writer-priority reader-writer locking with ownership checks;
- changed ticket-lock waiting to sleep/wakeup and added killed-waiter/owner cleanup;
- added `scounttest`, `pctest`, `rwtest`, and `tickettest` regressions.

### Verification

- added source, reconstructed-dist, and counter-mode build verification;
- added host-side repository regression tests;
- added QEMU cumulative smoke automation;
- added push/PR CI and a full six-cell counter matrix workflow.
