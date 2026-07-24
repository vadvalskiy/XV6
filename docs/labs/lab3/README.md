# Lab 3 — multi-level scheduling

Lab 3 extends the already modified kernel from Labs 1 and 2. No previous user
program or system call is replaced by a separate xv6 source copy.

## Implemented sections

- three queues selected by per-CPU weighted round robin;
- queue 0 round robin, queue 1 confidence-aware approximate SJF, and queue 2
  FCFS within its active queue slice;
- runnable-process aging and promotion;
- scheduling configuration and reporting system calls;
- deterministic syscall-free CPU workloads;
- runtime, runnable time, dispatch, preemption, response, and turnaround
  measurements returned by `waitstats`;
- `schedverify` marker: `LAB3 TEST PASS`.

## Reproducibility

Use `CPUS=1` for deterministic policy demonstrations. SMP scheduling outcomes
may differ in exact order while still satisfying queue and safety invariants.
