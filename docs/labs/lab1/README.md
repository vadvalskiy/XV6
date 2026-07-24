# Lab 1 — xv6 foundations and console editing

Lab 1 is the first cumulative phase. It modifies the shared repository-root source tree rather
than producing a separate operating-system copy.

## Implemented sections

- team identity in the `init` startup path;
- left/right and word-level cursor movement;
- selection, copy, paste, replacement, deletion, and deselection semantics;
- temporal undo for inserted characters;
- static command completion suitable for console interrupt context;
- checked `find_sum` parsing with optional signed mode and output path;
- `lab1test` regression marker: `LAB1 TEST PASS`.

## Evidence status

The source and image build are verified in the repository workflow. Historical
screenshots and observations in `original-report.fa.md` are retained as
originally reported evidence and are not relabeled as newly reproduced output.
