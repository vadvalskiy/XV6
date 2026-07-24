#include "types.h"
#include "stat.h"
#include "user.h"

static void
busy(int ticks)
{
  int start = uptime();
  volatile int x = getpid();
  int i;

  while(uptime() - start < ticks){
    for(i = 0; i < 25000; i++)
      x += i ^ getpid();
  }
}

int
main(int argc, char *argv[])
{
  int i, pid, donepid;
  int pfd[2];
  char go = 'x';

  printf(1, "workload_short: light CPU workload in RR queue\n");
  for(i = 0; i < 4; i++){
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
      busy(15 + i * 5);
      exit();
    }

    close(pfd[0]);
    if(set_scheduling_info(pid, 2 + i, 85) < 0)
      printf(2, "set_scheduling_info failed for pid %d\n", pid);
    if(change_queue(pid, 0) < 0)
      printf(2, "change_queue failed for pid %d\n", pid);
    if(write(pfd[1], &go, 1) != 1)
      printf(2, "release failed for pid %d\n", pid);
    close(pfd[1]);
  }

  print_scheduling_info();
  for(i = 0; i < 4; i++){
    donepid = wait();
    if(donepid >= 0)
      printf(1, "short child %d done\n", donepid);
  }
  printf(1, "workload_short done\n");
  exit();
}
