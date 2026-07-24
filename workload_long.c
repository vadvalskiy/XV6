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
  int units[4] = {700, 900, 1100, 1300};

  printf(1, "workload_long: deterministic CPU work in FCFS queue\n");
  for(i = 0; i < 4; i++){
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
    if(set_scheduling_info(pid, 8 + i * 3, 55 + i * 10) < 0 ||
       change_queue(pid, 2) < 0)
      printf(2, "scheduler configuration failed for pid %d\n", pid);
    printf(1, "configured child %d: queue=2 burst=%d work=%d units\n", pid, 8+i*3, units[i]);
    write(pfd[1], &go, 1);
    close(pfd[1]);
  }
  print_scheduling_info();
  for(i = 0; i < 4; i++){
    donepid = wait();
    if(donepid >= 0) printf(1, "long child %d done\n", donepid);
  }
  printf(1, "workload_long done\n");
  exit();
}
