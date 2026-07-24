# Lab 4 — multicore synchronization

Lab 4 is the fourth cumulative upgrade to the same kernel. All console,
system-call, sorting, and scheduling features from Labs 1–3 remain present.

## Implemented sections

- selectable global-unlocked, global-locked, and per-CPU syscall counters;
- total and per-CPU counter reporting;
- nonblocking and blocking bounded-buffer producer-consumer interfaces;
- writer-priority reader-writer locking with ownership validation;
- FIFO ticket locking without active spinning;
- cancellation and cleanup for killed waiters and exiting owners;
- deterministic tests with PASS markers for each synchronization subsystem.

## Counter modes

Compile with `EXTRA_CFLAGS=-DSYSCALL_COUNT_MODE=N`, where `N` is `0` for the
intentionally unsafe global counter, `1` for the spinlock-protected global
counter, or `2` for the default per-CPU implementation.
