# Lab 2 — system calls and kernel/user sorting

Lab 2 is applied directly after Lab 1 in the same repository-root source tree. The
console editor, startup identity, and `find_sum` utility therefore remain
available while the kernel/user ABI is extended.

## Implemented sections

- seeded pseudo-random number service with bounded user output;
- process relationship and state inspection;
- kernel-side checked file parsing and integer sorting;
- matched user-space sorting path;
- equivalent end-to-end timing scopes for both sorting programs;
- overflow, boundary, invalid-input, PRNG, and process-information regressions;
- `lab2test` marker: `LAB2 TEST PASS`.

## Measurement rule

Kernel and user sorting comparisons include the same read, parse, sort, create,
and write operations. Historical timing claims that used unequal scopes remain
archived but are not treated as verified benchmark evidence.
