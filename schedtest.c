#include "types.h"
#include "stat.h"
#include "user.h"

static void
burn_for_ticks(int target_ticks)
{
  int start;
  volatile int x;
  int i;

  start = uptime();
  x = 0;
  while(uptime() - start < target_ticks){
    for(i = 0; i < 50000; i++)
      x = x * 1664525 + i + getpid();
  }
}

int
main(int argc, char *argv[])
{
  int i, pid, donepid;
  int pfd[2];
  char go = 'x';
  int queues[6] = {0, 0, 1, 1, 2, 2};
  int bursts[6] = {8, 3, 2, 10, 4, 12};
  int confs[6] = {80, 70, 90, 40, 60, 95};
  int runticks[6] = {40, 30, 45, 35, 50, 55};

  printf(1, "schedtest: creating 6 CPU-bound children\n");
  printf(1, "schedtest: q0=RR, q1=approx-SJF, q2=FCFS\n");

  for(i = 0; i < 6; i++){
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
      burn_for_ticks(runticks[i]);
      exit();
    }

    close(pfd[0]);
    if(set_scheduling_info(pid, bursts[i], confs[i]) < 0)
      printf(2, "set_scheduling_info failed for pid %d\n", pid);
    if(change_queue(pid, queues[i]) < 0)
      printf(2, "change_queue failed for pid %d\n", pid);

    printf(1, "configured child %d: queue=%d burst=%d confidence=%d work=%d ticks\n",
           pid, queues[i], bursts[i], confs[i], runticks[i]);

    if(write(pfd[1], &go, 1) != 1)
      printf(2, "release failed for pid %d\n", pid);
    close(pfd[1]);
  }

  print_scheduling_info();
  for(i = 0; i < 6; i++){
    donepid = wait();
    if(donepid >= 0)
      printf(1, "child %d finished\n", donepid);
  }
  printf(1, "schedtest: all children finished\n");
  print_scheduling_info();
  exit();
}
