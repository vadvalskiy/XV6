#include "types.h"
#include "stat.h"
#include "user.h"

volatile int sink = 0;

static int
work(long iters)
{
  long i;
  for(i = 0; i < iters; i++){
    sink += (int)i;   // prevent optimizing away
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  int kids = 3;
  int t0 = 10, t1 = 20, t2 = 70;
  long iters = 200000000; // long run to reduce variance

  int pid;

  pid = fork();
  if(pid == 0){
    settickets(t0);
    work(iters);
    printf(1, "child A tickets=%d done\n", t0);
    exit();
  }

  pid = fork();
  if(pid == 0){
    settickets(t1);
    work(iters);
    printf(1, "child B tickets=%d done\n", t1);
    exit();
  }

  pid = fork();
  if(pid == 0){
    settickets(t2);
    work(iters);
    printf(1, "child C tickets=%d done\n", t2);
    exit();
  }

  // parent waits
  wait(); wait(); wait();
  printf(1, "parent done\n");
  exit();
}
