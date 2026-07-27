# Lab 2 — system calls and kernel services

Lab 2 extends the Lab 1 tree with a complete user-to-kernel system-call path and comparative kernel/user implementations.

## Implemented scope

- `setSeed` and `getRandomNumber` pseudo-random services;
- `process_information` process-tree inspection;
- checked kernel-side and user-side number sorting;
- dedicated programs for random, process, and sorting validation;
- `lab2test` regression marker: `LAB2 TEST PASS`.

## Primary source paths

`xv6/syscall.h`, `xv6/syscall.c`, `xv6/sysproc.c`, `xv6/sysfile.c`, `xv6/user.h`, `xv6/usys.S`, `xv6/dice.c`, `xv6/pinfo_test.c`, `xv6/sort_kernel.c`, `xv6/sort_user.c`, and `xv6/lab2test.c`.

## Evidence

- milestone: `lab2-complete`;
- report: [`docs/reports/lab-02/report.fa.md`](../reports/lab-02/report.fa.md) and [`report.fa.pdf`](../reports/lab-02/report.fa.pdf);
- assignment: [`docs/assignments/lab-02/`](../assignments/lab-02/).
