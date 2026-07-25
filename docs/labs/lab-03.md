# Lab 3 — multilevel process scheduling

Lab 3 replaces the baseline round-robin-only policy with a measured multilevel scheduler while preserving the earlier labs.

## Implemented scope

- queue-specific Round Robin, approximate SJF, and FCFS policies;
- Weighted Round Robin selection between queues;
- timer-driven quanta, wait/runtime accounting, and aging;
- scheduling control system calls and deterministic workload programs;
- `schedverify` regression marker: `SCHED VERIFY PASS`.

## Primary source paths

`xv6/proc.c`, `xv6/proc.h`, `xv6/trap.c`, `xv6/exec.c`, `xv6/schedstat.h`, `xv6/cpuwork.h`, `xv6/schedtest.c`, `xv6/schedverify.c`, and `xv6/workload_*.c`.

## Evidence

- milestone: `lab3-complete`;
- report: [`reports/lab-03/report.fa.md`](../../reports/lab-03/report.fa.md) and [`report.fa.pdf`](../../reports/lab-03/report.fa.pdf);
- assignment: [`docs/assignments/lab-03/`](../assignments/lab-03/).
