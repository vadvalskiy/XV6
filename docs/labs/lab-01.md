# Lab 1 — xv6 foundations and console editing

Lab 1 establishes the cumulative baseline and introduces the first user-visible extensions.

## Implemented scope

- team identity in the `init` startup path;
- cursor movement, word movement, selection, copy, paste, replacement, and deletion in `console.c`;
- temporal undo for inserted characters and static command completion;
- checked `find_sum` parsing and output handling;
- `lab1test` regression marker: `LAB1 TEST PASS`.

## Primary source paths

`xv6/init.c`, `xv6/console.c`, `xv6/kbd.c`, `xv6/kbd.h`, `xv6/find_sum.c`, `xv6/lab1test.c`, and `xv6/Makefile`.

## Evidence

- milestone: `lab1-complete`;
- report: [`reports/lab-01/report.fa.md`](../../reports/lab-01/report.fa.md) and [`report.fa.pdf`](../../reports/lab-01/report.fa.pdf);
- assignment: [`docs/assignments/lab-01/`](../assignments/lab-01/).
