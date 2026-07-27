#include "types.h"
#include "stat.h"
#include "user.h"

#define CAPACITY 16
#define NPROD 3
#define NCONS 3
#define ITEMS_PER_PRODUCER 20
#define TOTAL_ITEMS (NPROD * ITEMS_PER_PRODUCER)
#define ITEMS_PER_CONSUMER (TOTAL_ITEMS / NCONS)

int
main(int argc, char *argv[])
{
  int i, j, pid, value;
  int result_pipe[2];
  int seen[TOTAL_ITEMS];
  int failures = 0;

  while(try_consume(&value) == 0)
    ;
  if(try_produce(-1) < 0 || try_consume(&value) < 0 || value != -1){
    printf(2, "pctest: pointer-result API failed for value -1\n");
    failures++;
  }
  for(i = 0; i < CAPACITY; i++)
    if(try_produce(1000 + i) < 0) failures++;
  if(try_produce(9999) != -1){
    printf(2, "pctest: nonblocking full detection failed\n");
    failures++;
  }
  for(i = 0; i < CAPACITY; i++){
    if(try_consume(&value) < 0 || value != 1000 + i)
      failures++;
  }
  if(try_consume(&value) != -1){
    printf(2, "pctest: nonblocking empty detection failed\n");
    failures++;
  }

  if(pipe(result_pipe) < 0){
    printf(2, "pctest: result pipe failed\n");
    exit();
  }
  for(i = 0; i < TOTAL_ITEMS; i++) seen[i] = 0;

  for(i = 0; i < NCONS; i++){
    pid = fork();
    if(pid < 0){ failures++; break; }
    if(pid == 0){
      close(result_pipe[0]);
      for(j = 0; j < ITEMS_PER_CONSUMER; j++){
        if(consume_value(&value) < 0 || write(result_pipe[1], &value, sizeof(value)) != sizeof(value))
          exit();
      }
      close(result_pipe[1]);
      exit();
    }
  }

  for(i = 0; i < NPROD; i++){
    pid = fork();
    if(pid < 0){ failures++; break; }
    if(pid == 0){
      close(result_pipe[0]);
      close(result_pipe[1]);
      for(j = 0; j < ITEMS_PER_PRODUCER; j++)
        if(produce(i * ITEMS_PER_PRODUCER + j) < 0) exit();
      exit();
    }
  }

  close(result_pipe[1]);
  for(i = 0; i < NPROD + NCONS; i++) wait();
  for(i = 0; i < TOTAL_ITEMS; i++){
    if(read(result_pipe[0], &value, sizeof(value)) != sizeof(value)){
      failures++;
      break;
    }
    if(value < 0 || value >= TOTAL_ITEMS || seen[value]) failures++;
    else seen[value] = 1;
  }
  close(result_pipe[0]);
  for(i = 0; i < TOTAL_ITEMS; i++) if(!seen[i]) failures++;

  if(failures == 0)
    printf(1, "LAB4 PC TEST PASS\n");
  else
    printf(1, "LAB4 PC TEST FAIL: %d failure(s)\n", failures);
  exit();
}
