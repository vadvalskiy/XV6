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
  int units[6] = {220, 850, 280, 1050, 340, 650};
  int queue[6] = {0, 2, 1, 2, 1, 0};
  int burst[6] = {2, 12, 3, 14, 4, 8};

  printf(1, "workload_mixed: deterministic jobs across all queues\n");
  for(i = 0; i < 6; i++){
    if(pipe(pfd) < 0){ printf(2, "pipe failed\n"); exit(); }
    pid = fork();
    if(pid < 0){ printf(2, "fork failed\n"); exit(); }
    if(pid == 0){
      close(pfd[1]);
      if(read(pfd[0], &go, 1) != 1) exit();
      close(pfd[0]);
      cpu_work(units[i], units[i] > 600 ? 2 : 1);
      exit();
    }
    close(pfd[0]);
    if(set_scheduling_info(pid, burst[i], 100) < 0 || change_queue(pid, queue[i]) < 0)
      printf(2, "scheduler configuration failed for pid %d\n", pid);
    printf(1, "configured child %d: queue=%d burst=%d work=%d units\n",
           pid, queue[i], burst[i], units[i]);
    write(pfd[1], &go, 1);
    close(pfd[1]);
  }
  print_scheduling_info();
  for(i = 0; i < 6; i++){
    donepid = wait();
    if(donepid >= 0) printf(1, "mixed child %d done\n", donepid);
  }
  printf(1, "workload_mixed done\n");
  exit();
}
