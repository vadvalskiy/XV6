#include "types.h"
#include "stat.h"
#include "user.h"
#include "cpuwork.h"

int
main(int argc, char *argv[])
{
  int i, pid, donepid;
  int pfd[2];
  char go = 'x';
  int units[4] = {180, 240, 300, 360};

  printf(1, "workload_short: deterministic CPU work in RR queue\n");
  for(i = 0; i < 4; i++){
    if(pipe(pfd) < 0){
      printf(2, "pipe failed\n");
      exit();
    }
    pid = fork();
    if(pid < 0){
      printf(2, "fork failed\n");
      exit();
    }
    if(pid == 0){
      close(pfd[1]);
      if(read(pfd[0], &go, 1) != 1)
        exit();
      close(pfd[0]);
      cpu_work(units[i], 1);
      exit();
    }
    close(pfd[0]);
    if(set_scheduling_info(pid, 2 + i, 85) < 0 || change_queue(pid, 0) < 0)
      printf(2, "scheduler configuration failed for pid %d\n", pid);
    printf(1, "configured child %d: queue=0 burst=%d work=%d units\n", pid, 2+i, units[i]);
    write(pfd[1], &go, 1);
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
