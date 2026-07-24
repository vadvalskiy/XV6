#include "types.h"
#include "stat.h"
#include "user.h"

static void
busy(int ticks, int heavy)
{
  int start = uptime();
  volatile int x = getpid();
  int i, lim;

  lim = heavy ? 120000 : 30000;
  while(uptime() - start < ticks){
    for(i = 0; i < lim; i++)
      x = (x + (i * 3)) ^ getpid();
  }
}

int
main(int argc, char *argv[])
{
  int i, pid, donepid;
  int pfd[2];
  char go = 'x';
  int ticks[6] = {20, 65, 25, 75, 30, 55};
  int q[6] = {0, 2, 1, 2, 1, 0};
  int burst[6] = {2, 12, 3, 14, 4, 8};

  printf(1, "workload_mixed: short and long children across all queues\n");
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
      busy(ticks[i], ticks[i] > 50);
      exit();
    }

    close(pfd[0]);
    if(set_scheduling_info(pid, burst[i], 75) < 0)
      printf(2, "set_scheduling_info failed for pid %d\n", pid);
    if(change_queue(pid, q[i]) < 0)
      printf(2, "change_queue failed for pid %d\n", pid);
    if(write(pfd[1], &go, 1) != 1)
      printf(2, "release failed for pid %d\n", pid);
    close(pfd[1]);
  }

  print_scheduling_info();
  for(i = 0; i < 6; i++){
    donepid = wait();
    if(donepid >= 0)
      printf(1, "mixed child %d done\n", donepid);
  }
  printf(1, "workload_mixed done\n");
  exit();
}
