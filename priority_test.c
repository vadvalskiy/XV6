#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_CHILDREN 4
#define ITERATIONS 10000000

void do_work(char *name, int priority)
{
  volatile int i, j;
  volatile int x = 0;
  
  printf(1, "Process %s (PID %d, Priority %d) starting work\n", name, getpid(), priority);
  
  for(i = 0; i < ITERATIONS; i++){
    x = (x + i) % 1000;
    if(i % (ITERATIONS / 10) == 0){
      printf(1, "%s", name);
    }
  }
  
  printf(1, "\nProcess %s (PID %d) finished work. Final x = %d\n", name, getpid(), x);
}

int main(void)
{
  int pids[NUM_CHILDREN];
  int priorities[NUM_CHILDREN] = {5, 1, 3, 3};
  char *names[NUM_CHILDREN] = {"P1", "P2", "P3", "P4"};
  int i;
  int pipe_fd[2];
  
  printf(1, "\nPriority Scheduler Test\n");
  printf(1, "Parent PID: %d\n", getpid());
  
  if(pipe(pipe_fd) < 0){
    printf(1, "Pipe creation failed\n");
    exit();
  }
  
  for(i = 0; i < NUM_CHILDREN; i++){
    pids[i] = fork();
    
    if(pids[i] < 0){
      printf(1, "Fork failed\n");
      exit();
    }
    
    if(pids[i] == 0){
      char buf[1];
      
      close(pipe_fd[1]);
      
      printf(1, "Child %s created with PID %d, waiting for start signal...\n", names[i], getpid());
      
      read(pipe_fd[0], buf, 1);
      close(pipe_fd[0]);
      
      do_work(names[i], priorities[i]);
      exit();
    }
  }
  
  sleep(2);
  
  printf(1, "\nSetting priorities:\n");
  for(i = 0; i < NUM_CHILDREN; i++){
    if(setpriority(pids[i], priorities[i]) < 0){
      printf(1, "Failed to set priority for PID %d\n", pids[i]);
    } else {
      printf(1, "Set priority of PID %d (child %s) to %d\n", pids[i], names[i], priorities[i]);
    }
  }
  
  printf(1, "\nExpected behavior:\n");
  printf(1, "  - P2 (priority 1) should run most often (highest priority)\n");
  printf(1, "  - P3 and P4 (priority 3) should alternate (round-robin tie)\n");
  printf(1, "  - P1 (priority 5) should run least often\n\n");
  
  printf(1, "Observed output (watch the letters):\n");
  printf(1, "\n");
  

  close(pipe_fd[0]);
  
  for(i = 0; i < NUM_CHILDREN; i++){
    write(pipe_fd[1], "G", 1);
  }
  close(pipe_fd[1]);
  
  for(i = 0; i < NUM_CHILDREN; i++){
    wait();
  }
  
  printf(1, "\nTest Complete\n");
  exit();
}