#include "types.h"
#include "stat.h"
#include "user.h"

void cpu_bound(char letter, int priority)
{
  volatile int i, j;
  volatile long long x = 0;
  
  printf(1, "[%c] PID=%d priority=%d starting\n", letter, getpid(), priority);
  
  for(i = 0; i < 100; i++){
    printf(1, "%c", letter);
    
    for(j = 0; j < 500000; j++){
      x = (x + j) & 0x7FFFFFFF;
    }
    
    yield();
  }
  
  printf(1, "[%c] done ", letter);
}

int main(int argc, char *argv[])
{
  int pid1, pid2, pid3;
  
  printf(1, "Testing Priority Scheduler with Different Priorities\n");
  printf(1, "\n");
  
  if((pid1 = fork()) == 0){
    sleep(20);
    cpu_bound('H', 0);
    exit();
  }
  
  if((pid2 = fork()) == 0){
    sleep(20);
    cpu_bound('M', 5);
    exit();
  }
  
  if((pid3 = fork()) == 0){
    sleep(20);
    cpu_bound('L', 10);
    exit();
  }
  
  sleep(10);
  
  setpriority(pid1, 0);
  setpriority(pid2, 5);
  setpriority(pid3, 10);
  
  printf(1, "\nPriorities set:\n");
  printf(1, "  PID %d: priority 0 (highest)\n", pid1);
  printf(1, "  PID %d: priority 5 (medium)\n", pid2);
  printf(1, "  PID %d: priority 10 (lowest)\n", pid3);
  printf(1, "\nExpected: 'H' should appear most frequently\n");
  printf(1, "Actual output:\n");
  
  wait();
  wait();
  wait();
  
  printf(1, "\nTest complete.\n");
  exit();
}