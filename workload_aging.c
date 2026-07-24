#include "types.h"
#include "stat.h"
#include "user.h"

#define NCHILD 4

static void
busy(int ticks)
{
  int start = uptime();
  volatile int x = getpid();
  int i;

  while(uptime() - start < ticks){
    for(i = 0; i < 90000; i++)
      x = (x * 1103515245) + i + getpid();
  }
}

static char*
role_for_pid(int pid, int pids[])
{
  if(pid == pids[0])
    return "fcfs hog";
  return "waiting job";
}

int
main(int argc, char *argv[])
{
  int i, pid, donepid;
  int pfd[2];
  int wfds[NCHILD];
  int pids[NCHILD];
  char go = 'x';
  int runticks[NCHILD] = {1100, 40, 40, 40};
  int bursts[NCHILD] = {100, 4, 5, 6};

  printf(1, "workload_aging: run with CPUS=1 to demonstrate aging clearly\n");
  printf(1, "workload_aging: one long FCFS job keeps three FCFS jobs waiting\n");
  printf(1, "workload_aging: expect kernel lines: aging: pid X promoted from queue 2 to queue 1\n");

  for(i = 0; i < NCHILD; i++){
    if(pipe(pfd) < 0){
      printf(2, "pipe failed\n");
      exit();
    }

    pid = fork();
    if(pid < 0){
      printf(2, "fork failed\n");
      close(pfd[0]);
      close(pfd[1]);
      exit();
    }

    if(pid == 0){
      close(pfd[1]);
      if(read(pfd[0], &go, 1) != 1)
        exit();
      close(pfd[0]);
      busy(runticks[i]);
      exit();
    }

    close(pfd[0]);
    pids[i] = pid;
    wfds[i] = pfd[1];
    if(set_scheduling_info(pid, bursts[i], 70) < 0)
      printf(2, "set_scheduling_info failed for pid %d\n", pid);
    if(change_queue(pid, 2) < 0)
      printf(2, "change_queue failed for pid %d\n", pid);
    printf(1, "configured aging child %d: queue=2 burst=%d work=%d ticks\n",
           pid, bursts[i], runticks[i]);
  }

  print_scheduling_info();
  printf(1, "workload_aging: releasing children now\n");
  for(i = 0; i < NCHILD; i++){
    if(write(wfds[i], &go, 1) != 1)
      printf(2, "release failed for pid %d\n", pids[i]);
    close(wfds[i]);
  }

  for(i = 0; i < NCHILD; i++){
    donepid = wait();
    if(donepid >= 0)
      printf(1, "aging child %d done (%s)\n", donepid, role_for_pid(donepid, pids));
  }
  printf(1, "workload_aging done\n");
  print_scheduling_info();
  exit();
}
