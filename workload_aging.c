#include "types.h"
#include "stat.h"
#include "user.h"
#include "cpuwork.h"

#define NCHILD 4

int
main(int argc, char *argv[])
{
  int i, pid, donepid;
  int pfd[2];
  int wfds[NCHILD];
  char go = 'x';
  int work_units[NCHILD] = {5000, 250, 250, 250};
  int bursts[NCHILD] = {100, 4, 5, 6};

  printf(1, "workload_aging: use CPUS=1; first FCFS job is deliberately long\n");
  printf(1, "workload_aging: expected kernel promotion messages after %d ticks\n", 800);
  for(i = 0; i < NCHILD; i++){
    if(pipe(pfd) < 0){ printf(2, "pipe failed\n"); exit(); }
    pid = fork();
    if(pid < 0){ printf(2, "fork failed\n"); exit(); }
    if(pid == 0){
      close(pfd[1]);
      if(read(pfd[0], &go, 1) != 1) exit();
      close(pfd[0]);
      cpu_work(work_units[i], i == 0 ? 3 : 1);
      exit();
    }
    close(pfd[0]);
    wfds[i] = pfd[1];
    if(set_scheduling_info(pid, bursts[i], 100) < 0 || change_queue(pid, 2) < 0)
      printf(2, "scheduler configuration failed for pid %d\n", pid);
    printf(1, "configured child %d: queue=2 burst=%d work=%d units\n",
           pid, bursts[i], work_units[i]);
  }
  print_scheduling_info();
  for(i = 0; i < NCHILD; i++){
    write(wfds[i], &go, 1);
    close(wfds[i]);
  }
  for(i = 0; i < NCHILD; i++){
    donepid = wait();
    if(donepid >= 0) printf(1, "aging child %d done\n", donepid);
  }
  printf(1, "workload_aging done\n");
  exit();
}
