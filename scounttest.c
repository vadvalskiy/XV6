#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscall.h"
#include "param.h"

#define DEFAULT_CHILDREN 8
#define DEFAULT_ITERATIONS 10000

int
main(int argc, char *argv[])
{
  int children;
  int iterations;
  int before_total;
  int after_total;
  int before_cpu[NCPU];
  int after_cpu;
  int pid;
  int i;
  int j;
  int expected;

  children = DEFAULT_CHILDREN;
  iterations = DEFAULT_ITERATIONS;
  if(argc >= 2)
    children = atoi(argv[1]);
  if(argc >= 3)
    iterations = atoi(argv[2]);
  if(children < 1)
    children = 1;
  if(children > 32)
    children = 32;
  if(iterations < 1)
    iterations = 1;

  before_total = getcount(SYS_getpid);
  for(i = 0; i < NCPU; i++)
    before_cpu[i] = getcpucount(i, SYS_getpid);

  for(i = 0; i < children; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      for(j = 0; j < iterations; j++)
        getpid();
      exit();
    }
  }

  for(i = 0; i < children; i++)
    wait();

  after_total = getcount(SYS_getpid);
  expected = children * iterations;

  printf(1, "syscall counter test\n");
  printf(1, "children: %d iterations: %d\n", children, iterations);
  for(i = 0; i < NCPU; i++){
    after_cpu = getcpucount(i, SYS_getpid);
    printf(1, "CPU %d getpid count: %d\n", i, after_cpu - before_cpu[i]);
  }
  printf(1, "Total getpid count: %d\n", after_total - before_total);
  printf(1, "Expected count: %d\n", expected);

  exit();
}
