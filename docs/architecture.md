# Cumulative architecture

## Design invariant

The repository has exactly one executable xv6 source tree, located directly at the repository root. Every laboratory phase extends that tree and must preserve all previously introduced interfaces and user programs.

```text
baseline commit
  └── Lab 1 cumulative point
        └── Lab 2 cumulative point
              └── Lab 3 cumulative point
                    └── Lab 4 cumulative point
                          └── release-ready repository tooling and CI
```

The files under `docs/labs/` describe phase requirements and archived evidence. They do not contain buildable kernel copies.

## Baseline kernel

The project begins from MIT's xv6-public x86 kernel, retaining its process, virtual-memory, filesystem, trap, interrupt, and user-library architecture.

## Lab 1 layer

### Console input path

`kbd.c` translates keyboard scan codes and `console.c` owns the editable input state. The cumulative editor supports:

- character and word cursor movement;
- selection, copy, paste, deletion, and replacement;
- bounded undo for inserted input;
- static command completion.

Completion is static by design because performing filesystem traversal while handling console input can violate kernel-context and locking constraints.

### User utility

`find_sum.c` parses checked integers, supports signed or unsigned interpretation, and writes a result file. `lab1test.c` exercises numeric boundaries, invalid input, and result behavior without replacing the interactive console tests that require manual key input.

## Lab 2 layer

### System-call ABI

The cumulative ABI adds:

- `setSeed` and `getRandomNumber`;
- `process_information`;
- `sort_numbers`.

The dispatch table is in `syscall.c`; kernel handlers are in `sysproc.c` and `sysfile.c`; user declarations and assembly stubs are in `user.h` and `usys.S`.

### Sorting comparison

`sort_kernel.c` requests kernel-side sorting while `sort_user.c` performs the corresponding user-space path. Both timing scopes cover read, parse, sort, output creation, and output writing so their reported elapsed ticks refer to equivalent end-to-end work.

## Lab 3 layer

### Scheduler state

`proc.h` extends each process with queue, burst estimate, confidence, aging, dispatch, response, runtime, runnable-time, and preemption fields. `proc.c` implements:

- queue selection through per-CPU weighted round robin;
- queue 0 round robin;
- queue 1 confidence-aware approximate shortest-job-first selection;
- queue 2 FCFS selection within its active weighted slice;
- runnable-process aging and promotion;
- statistics collection and `waitstats` return values.

### Timer coupling

`trap.c` accounts running and runnable ticks, updates burst measurements, and triggers queue-specific quantum preemption. `cpuwork.h` provides deterministic arithmetic work so tests measure CPU work rather than wall-clock waiting.

## Lab 4 layer

### Syscall counters

`SYSCALL_COUNT_MODE` selects one of three compile-time implementations:

| Value | Implementation | Purpose |
| --- | --- | --- |
| `0` | shared global counter without locking | intentionally unsafe race baseline |
| `1` | shared global counter protected by a spinlock | correct shared-state baseline |
| `2` | aligned per-CPU counters with aggregated reads | default low-contention implementation |

`getcountmode`, `getcount`, and `getcpucount` expose the experiment to `scounttest`.

### Bounded buffer

The kernel exposes both nonblocking (`try_produce`, `try_consume`) and blocking (`produce`, `consume_value`) interfaces. Status and data are separated so all integer values, including `-1`, can be transferred without ambiguity. Sleep/wakeup transitions use the shared buffer lock to avoid lost wakeups.

### Reader-writer lock

The reader-writer service gives priority to waiting writers. Per-process ownership state prevents a process from releasing a lock it did not acquire and supports cleanup on exit.

### FIFO ticket lock

Ticket allocation is monotonic and FIFO. Waiters sleep instead of actively spinning. Cancellation state allows killed waiters to stop blocking the queue, and process-exit cleanup prevents an exiting owner from permanently retaining the lock.

## Build and verification boundary

The root task runner separates four evidence levels:

1. repository checks and host regressions;
2. source and distribution compilation;
3. QEMU guest smoke tests;
4. repeated benchmark/experimental interpretation.

Only levels actually executed may be described as verified. See `docs/verification.md`.
