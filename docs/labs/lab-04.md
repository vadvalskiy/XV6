# Lab 4 — multicore synchronization

Lab 4 adds synchronization experiments and kernel services to the complete Lab 1–3 system.

## Implemented scope

- syscall counters in unlocked, globally locked, and per-CPU modes;
- bounded producer-consumer buffer using `sleep` and `wakeup`;
- writer-preference reader-writer lock;
- FIFO ticket lock and cancellation-safe test paths;
- guest regressions: `scounttest`, `pctest`, `rwtest`, and `tickettest`.

## Primary source paths

`xv6/param.h`, `xv6/syscall.c`, `xv6/sysproc.c`, `xv6/main.c`, `xv6/scounttest.c`, `xv6/pctest.c`, `xv6/rwtest.c`, and `xv6/tickettest.c`.

## Evidence

- milestone: `lab4-complete`;
- report: [`docs/reports/lab-04/report.fa.md`](../reports/lab-04/report.fa.md) and [`report.fa.pdf`](../reports/lab-04/report.fa.pdf);
- assignment: [`docs/assignments/lab-04/`](../assignments/lab-04/).
