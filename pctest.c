#include "types.h"
#include "stat.h"
#include "user.h"

#define NPROD 3
#define NCONS 3
#define ITEMS_PER_PRODUCER 20
#define ITEMS_PER_CONSUMER ((NPROD * ITEMS_PER_PRODUCER) / NCONS)

int
main(int argc, char *argv[])
{
  int i;
  int j;
  int pid;
  int value;
  int base;

  printf(1, "producer-consumer test: blocking sleep/wakeup version\n");

  for(i = 0; i < NCONS; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      for(j = 0; j < ITEMS_PER_CONSUMER; j++){
        value = consume();
        printf(1, "consumer %d consumed %d\n", i + 1, value);
      }
      exit();
    }
  }

  sleep(10);

  for(i = 0; i < NPROD; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      base = (i + 1) * 100;
      for(j = 0; j < ITEMS_PER_PRODUCER; j++){
        value = base + j;
        if(produce(value) < 0){
          printf(1, "producer %d failed to produce %d\n", i + 1, value);
          exit();
        }
        printf(1, "producer %d produced %d\n", i + 1, value);
      }
      exit();
    }
  }

  for(i = 0; i < NPROD + NCONS; i++)
    wait();

  printf(1, "producer-consumer test done\n");
  exit();
}
