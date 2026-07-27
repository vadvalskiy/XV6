# Security policy

## Scope

This repository is an educational xv6/x86 kernel. It is not production software and must not be used to isolate untrusted workloads, protect sensitive data, or provide security guarantees.

The Lab 4 `SYSCALL_COUNT_MODE=0` implementation is intentionally unsynchronized and demonstrates a race condition. Its presence is a documented experimental baseline rather than a supported safe configuration.

## Reporting

General reproducible defects may be reported through GitHub issues. For potentially sensitive findings, use GitHub private vulnerability reporting when it is enabled for the repository, or contact the repository owner through their GitHub profile before disclosing technical details publicly.

Include the commit hash, host and toolchain details, QEMU version, CPU count, reproduction commands, and complete serial output. Do not include credentials or personal data in public reports.

## Supported state

Only the current `main` branch and tagged cumulative phase points are maintained. Submitted reports are archival course artifacts and are not supported executable releases.
