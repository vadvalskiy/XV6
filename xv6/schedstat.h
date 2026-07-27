#ifndef XV6_SCHEDSTAT_H
#define XV6_SCHEDSTAT_H

// Stable user/kernel ABI for scheduler measurements returned by waitstats().
struct sched_stats {
  int pid;
  int queue;
  uint created_tick;
  uint first_run_tick;
  uint exit_tick;
  uint runtime_ticks;
  uint runnable_ticks;
  uint dispatches;
  uint preemptions;
};

#endif
