#include "types.h"
#include "stat.h"
#include "user.h"

void worker(char letter)
{
  volatile int i, j;
  volatile long x = 0;
  
  for(i = 0; i < 20; i++){
    printf(1, "%c", letter);
    
    for(j = 0; j < 5000000; j++){
      x = (x + j) & 0x7FFFFFFF;
    }
  }
  printf(1, "[%c done] ", letter);
}

int main(void)
{
  int i, pids[3];
  char *letters = "ABC";
  
  printf(1, "\nRound-Robin Tie Breaking Test (All priority = 5)\n");
  printf(1, "\n");
  
  for(i = 0; i < 3; i++){
    if((pids[i] = fork()) == 0){
      sleep(20);
      worker(letters[i]);
      exit();
    }
  }
  
  sleep(10);
  
  for(i = 0; i < 3; i++){
    setpriority(pids[i], 5);
  }
  
  printf(1, "All processes have priority 5 (equal)\n");
  printf(1, "Expected: A, B, C should interleave (round-robin)\n");
  printf(1, "Actual output:\n");
  
  for(i = 0; i < 3; i++){
    wait();
  }
  
  printf(1, "\nTest complete.\n");
  exit();
}