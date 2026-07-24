#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscall.h"
#include "param.h"

#define DEFAULT_CHILDREN 8
#define DEFAULT_ITERATIONS 10000

static char*
mode_name(int mode)
{
  if(mode == SYSCALL_COUNT_GLOBAL_UNLOCKED) return "global-unlocked";
  if(mode == SYSCALL_COUNT_GLOBAL_LOCKED) return "global-locked";
  if(mode == SYSCALL_COUNT_PERCPU) return "per-cpu";
  return "unknown";
}

int
main(int argc, char *argv[])
{
  int children = DEFAULT_CHILDREN;
  int iterations = DEFAULT_ITERATIONS;
  int before_total, after_total, expected, actual;
  int before_cpu[NCPU];
  int mode, pid, i, j;
  uint t0, t1;
  int failures = 0;

  if(argc >= 2) children = atoi(argv[1]);
  if(argc >= 3) iterations = atoi(argv[2]);
  if(children < 1) children = 1;
  if(children > 32) children = 32;
  if(iterations < 1) iterations = 1;

  mode = getcountmode();
  before_total = getcount(SYS_getpid);
  for(i = 0; i < NCPU; i++)
    before_cpu[i] = mode == SYSCALL_COUNT_PERCPU ? getcpucount(i, SYS_getpid) : -1;

  t0 = uptime();
  for(i = 0; i < children; i++){
    pid = fork();
    if(pid < 0){ failures++; break; }
    if(pid == 0){
      for(j = 0; j < iterations; j++)
        getpid();
      exit();
    }
  }
  for(j = 0; j < i; j++)
    wait();
  t1 = uptime();

  after_total = getcount(SYS_getpid);
  expected = i * iterations;
  actual = after_total - before_total;

  printf(1, "counter mode: %s (%d)\n", mode_name(mode), mode);
  printf(1, "children=%d iterations=%d elapsed=%d ticks\n", i, iterations, t1-t0);
  if(mode == SYSCALL_COUNT_PERCPU){
    int sum = 0;
    for(j = 0; j < NCPU; j++){
      int after = getcpucount(j, SYS_getpid);
      int delta = after - before_cpu[j];
      sum += delta;
      printf(1, "CPU %d getpid delta: %d\n", j, delta);
    }
    if(sum != actual){
      printf(2, "scounttest: per-CPU sum mismatch\n");
      failures++;
    }
  } else if(getcpucount(0, SYS_getpid) != -1){
    printf(2, "scounttest: shared mode exposed misleading per-CPU data\n");
    failures++;
  }

  printf(1, "actual=%d expected=%d\n", actual, expected);
  if(mode == SYSCALL_COUNT_GLOBAL_UNLOCKED){
    if(actual <= 0 || actual > expected)
      failures++;
    printf(1, "unlocked mode permits lost increments by design\n");
  } else if(actual != expected){
    failures++;
  }

  if(failures == 0)
    printf(1, "LAB4 COUNTER TEST PASS\n");
  else
    printf(1, "LAB4 COUNTER TEST FAIL: %d failure(s)\n", failures);
  exit();
}
