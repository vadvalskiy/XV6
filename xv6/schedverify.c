#include "types.h"
#include "stat.h"
#include "user.h"
#include "schedstat.h"
#include "cpuwork.h"

#define NCHILD 3

int
main(int argc, char *argv[])
{
  int i, pid;
  int failures = 0;
  int pfd[2];
  int wfds[NCHILD];
  char go = 'x';
  int queues[NCHILD] = {0, 1, 2};
  int bursts[NCHILD] = {5, 2, 8};
  struct sched_stats stats;
  uint total_preemptions = 0;

  if(change_queue(getpid(), -1) != -1 ||
     set_scheduling_info(getpid(), 0, 50) != -1 ||
     set_scheduling_info(getpid(), 1, 101) != -1){
    printf(2, "schedverify: invalid parameter validation failed\n");
    failures++;
  }

  for(i = 0; i < NCHILD; i++){
    if(pipe(pfd) < 0){ failures++; break; }
    pid = fork();
    if(pid < 0){ failures++; break; }
    if(pid == 0){
      close(pfd[1]);
      if(read(pfd[0], &go, 1) != 1) exit();
      close(pfd[0]);
      cpu_work(900 + i * 150, 2);
      exit();
    }
    close(pfd[0]);
    wfds[i] = pfd[1];
    if(set_scheduling_info(pid, bursts[i], 100) < 0 || change_queue(pid, queues[i]) < 0){
      printf(2, "schedverify: child configuration failed\n");
      failures++;
    }
  }

  for(i = 0; i < NCHILD; i++){
    write(wfds[i], &go, 1);
    close(wfds[i]);
  }

  for(i = 0; i < NCHILD; i++){
    pid = waitstats(&stats);
    if(pid < 0){
      printf(2, "schedverify: waitstats failed\n");
      failures++;
      break;
    }
    if(stats.pid != pid || stats.first_run_tick == ~0U ||
       stats.dispatches == 0 || stats.runtime_ticks == 0 ||
       (int)(stats.exit_tick - stats.created_tick) < 0){
      printf(2, "schedverify: invalid metrics for pid %d\n", pid);
      failures++;
    }
    total_preemptions += stats.preemptions;
    printf(1, "schedverify metric: pid=%d q=%d run=%d ready=%d dispatch=%d preempt=%d\n",
           stats.pid, stats.queue, stats.runtime_ticks, stats.runnable_ticks,
           stats.dispatches, stats.preemptions);
  }

  if(total_preemptions == 0){
    printf(2, "schedverify: no timer preemption observed\n");
    failures++;
  }

  if(failures == 0)
    printf(1, "LAB3 TEST PASS\n");
  else
    printf(1, "LAB3 TEST FAIL: %d failure(s)\n", failures);
  exit();
}
