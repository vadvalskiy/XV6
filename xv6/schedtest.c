#include "types.h"
#include "stat.h"
#include "user.h"
#include "schedstat.h"
#include "cpuwork.h"

int
main(int argc, char *argv[])
{
  int i, pid;
  int pfd[2];
  char go = 'x';
  int queues[6] = {0, 0, 1, 1, 2, 2};
  int bursts[6] = {8, 3, 2, 10, 4, 12};
  int confs[6] = {80, 70, 100, 100, 60, 95};
  int units[6] = {650, 500, 700, 600, 850, 950};
  struct sched_stats stats;

  printf(1, "schedtest: deterministic CPU-bound children\n");
  printf(1, "pid queue runtime runnable dispatch preempt response turnaround\n");
  for(i = 0; i < 6; i++){
    if(pipe(pfd) < 0){ printf(2, "pipe failed\n"); exit(); }
    pid = fork();
    if(pid < 0){ printf(2, "fork failed\n"); exit(); }
    if(pid == 0){
      close(pfd[1]);
      if(read(pfd[0], &go, 1) != 1) exit();
      close(pfd[0]);
      cpu_work(units[i], 2);
      exit();
    }
    close(pfd[0]);
    if(set_scheduling_info(pid, bursts[i], confs[i]) < 0 || change_queue(pid, queues[i]) < 0)
      printf(2, "scheduler configuration failed for pid %d\n", pid);
    write(pfd[1], &go, 1);
    close(pfd[1]);
  }
  print_scheduling_info();
  for(i = 0; i < 6; i++){
    pid = waitstats(&stats);
    if(pid < 0){ printf(2, "waitstats failed\n"); break; }
    printf(1, "%d %d %d %d %d %d %d %d\n",
           stats.pid, stats.queue, stats.runtime_ticks, stats.runnable_ticks,
           stats.dispatches, stats.preemptions,
           stats.first_run_tick - stats.created_tick,
           stats.exit_tick - stats.created_tick);
  }
  printf(1, "schedtest done\n");
  exit();
}
