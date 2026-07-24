#include "types.h"
#include "stat.h"
#include "user.h"

#define EARLY_READERS 3
#define LATE_READERS 3

static void
reader_body(int id, int late)
{
  if(late)
    printf(1, "late reader %d requests read lock\n", id);
  else
    printf(1, "early reader %d requests read lock\n", id);

  if(rw_acquire_read() < 0){
    printf(1, "reader %d failed to acquire read lock\n", id);
    exit();
  }

  if(late)
    printf(1, "late reader %d ENTERS after writer priority check\n", id);
  else
    printf(1, "early reader %d ENTERS\n", id);

  sleep(late ? 10 : 60);

  if(late)
    printf(1, "late reader %d leaves\n", id);
  else
    printf(1, "early reader %d leaves\n", id);

  rw_release_read();
  exit();
}

int
main(int argc, char *argv[])
{
  int i;
  int pid;

  printf(1, "reader-writer lock test: writer priority\n");

  for(i = 0; i < EARLY_READERS; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0)
      reader_body(i + 1, 0);
  }

  sleep(10);

  pid = fork();
  if(pid < 0){
    printf(1, "fork failed\n");
    exit();
  }
  if(pid == 0){
    printf(1, "writer requests write lock\n");
    if(rw_acquire_write() < 0){
      printf(1, "writer failed to acquire write lock\n");
      exit();
    }
    printf(1, "writer ENTERS\n");
    sleep(30);
    printf(1, "writer leaves\n");
    rw_release_write();
    exit();
  }

  sleep(5);

  for(i = 0; i < LATE_READERS; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0)
      reader_body(i + 1, 1);
  }

  for(i = 0; i < EARLY_READERS + LATE_READERS + 1; i++)
    wait();

  printf(1, "reader-writer test done\n");
  exit();
}
