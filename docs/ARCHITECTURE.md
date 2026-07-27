# Cumulative architecture

## Design invariant

The repository has exactly one executable operating-system source tree: `xv6/`. Every laboratory phase extends that tree and preserves the interfaces and guest programs introduced by earlier phases.

```text
upstream xv6/x86 fork
  └── Lab 1 cumulative point
        └── Lab 2 cumulative point
              └── Lab 3 cumulative point
                    └── Lab 4 cumulative point
                          └── public documentation and verification tooling
```

Reports and assignment specifications are documentation artifacts. They do not contain alternative buildable kernels.

## Repository boundary

The root repository provides project-level documentation, automation, tests, and CI. The `xv6/` directory preserves the internally flat layout expected by the historical xv6 Makefile and source includes.

```text
repository task runner -> make -C xv6 ...
host verification      -> clean temporary copies of xv6/
guest verification     -> one cumulative fs.img and xv6.img
```

This boundary keeps the public repository navigable without introducing per-Lab source duplication.

## Baseline kernel

The source is derived from MIT's historical `xv6-public` x86 teaching kernel. The original process, virtual-memory, filesystem, trap, interrupt, and user-library architecture remains the base for all four laboratory phases.

## Lab 1 layer

`xv6/kbd.c` translates keyboard scan codes and `xv6/console.c` owns editable input state. The cumulative editor supports character and word movement, selection, copy/paste, deletion, replacement, bounded undo, and static command completion.

`xv6/find_sum.c` performs checked integer parsing and file output. `xv6/lab1test.c` covers numeric and file-handling regressions; interactive keyboard behavior remains a guest/manual boundary.

## Lab 2 layer

The user-to-kernel ABI adds `setSeed`, `getRandomNumber`, `process_information`, and `sort_numbers`. Dispatch is registered in `xv6/syscall.c`; handlers reside in `xv6/sysproc.c` and `xv6/sysfile.c`; declarations and assembly stubs are exposed through `xv6/user.h` and `xv6/usys.S`.

`xv6/sort_kernel.c` and `xv6/sort_user.c` compare equivalent end-to-end paths covering input, parsing, sorting, output creation, and output writing.

## Lab 3 layer

`xv6/proc.h` extends process state with queue, burst estimate, confidence, aging, dispatch, response, runtime, runnable-time, and preemption data. `xv6/proc.c` implements queue-specific RR, approximate SJF, FCFS, per-CPU weighted queue selection, and runnable-process aging.

`xv6/trap.c` couples timer interrupts to accounting and queue-specific preemption. `xv6/cpuwork.h` supplies deterministic arithmetic workloads, while `xv6/schedstat.h` defines the measurement ABI used by scheduler regressions.

## Lab 4 layer

### Syscall counters

`SYSCALL_COUNT_MODE` selects an intentionally unsafe global counter (`0`), a spinlock-protected global counter (`1`), or aligned per-CPU counters (`2`, default).

### Bounded buffer

Kernel producer-consumer services separate status from transferred values and use `sleep`/`wakeup` under the shared buffer lock to avoid active waiting and lost wakeups.

### Reader-writer lock

The reader-writer service gives priority to waiting writers. Per-process ownership state prevents invalid release and supports process-exit cleanup.

### FIFO ticket lock

Ticket allocation is monotonic and FIFO. Waiters sleep instead of spinning, and cancellation/exit handling prevents abandoned tickets or owners from permanently blocking progress.

## Documentation and evidence map

- `docs/labs/`: phase summaries and source-path maps;
- `docs/assignments/`: original requirements and searchable transcriptions;
- `docs/reports/`: complete submitted Markdown/PDF reports;
- `docs/VERIFICATION.md`: evidence labels, commands, and success criteria;
- `scripts/` and `tests/`: executable host and guest verification paths.

Only evidence that was actually executed under the stated environment may be labeled verified.
