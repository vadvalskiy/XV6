#include "types.h"
#include "stat.h"
#include "user.h"

static void
make_grandchildren_and_report(void)
{
  int i, pid;

  for(i = 0; i < 2; i++){
    pid = fork();
    if(pid < 0){
      printf(2, "grandchild fork failed\n");
      exit();
    }
    if(pid == 0){
      sleep(180);
      printf(1, "=== grandchild %d info ===\n", getpid());
      process_information(getpid());
      exit();
    }
  }

  sleep(40);
  printf(1, "=== child-with-children %d info ===\n", getpid());
  process_information(getpid());
  wait();
  wait();
  exit();
}

int
main(void)
{
  int i, pid;

  for(i = 0; i < 3; i++){
    pid = fork();
    if(pid < 0){
      printf(2, "fork failed\n");
      exit();
    }
    if(pid == 0){
      if(i == 0)
        make_grandchildren_and_report();
      sleep(120 + i * 20);
      printf(1, "=== child %d info ===\n", getpid());
      process_information(getpid());
      exit();
    }
  }

  sleep(30);
  printf(1, "=== parent %d info ===\n", getpid());
  process_information(getpid());

  for(i = 0; i < 3; i++)
    wait();

  exit();
}
