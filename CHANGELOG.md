# Changelog

This academic repository does not use artificial semantic versions. Changes are organized by cumulative laboratory phase and by the reconstructed Git history.

## 25 July 2026 — cumulative repository reconstruction

### Repository architecture

- replaced parallel per-Lab xv6 copies with one cumulative xv6 source tree at the repository root;
- preserved assignment specifications and original reports under `docs/labs/`;
- added an MRS-RS-compliant root structure, task runner, notices, and maintenance documentation;
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
