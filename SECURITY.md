# Security policy

## Scope

This repository is an educational xv6/x86 kernel. It is not production software and must not be used to isolate untrusted workloads, protect sensitive data, or provide security guarantees.

The Lab 4 `SYSCALL_COUNT_MODE=0` implementation is intentionally unsynchronized and demonstrates a race condition. Its presence is not a vulnerability in the project scope; it is a documented experimental baseline.

## Reporting

Potential defects that violate the documented educational invariants may be reported privately to:

- `mragetsars@gmail.com`
- `meraj.prhosseiny@ut.ac.ir`
- `ali.sadeghi.m@ut.ac.ir`

Include the commit hash, host/toolchain details, QEMU version, CPU count, reproduction commands, and complete serial output. Do not include credentials or personal data in a public issue.

## Supported state

Only the current `main` branch and tagged cumulative phase points are maintained. Historical assignment reports are archival evidence and are not supported executable releases.
