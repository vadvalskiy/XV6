#include "types.h"
#include "stat.h"
#include "user.h"

#define NCHILD 5

int
main(int argc, char *argv[])
{
  int i;
  int pid;
  int ticket;

  printf(1, "ticket lock test: FIFO order\n");

  for(i = 0; i < NCHILD; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      exit();
    }
    if(pid == 0){
      sleep(i * 2);
      printf(1, "child pid %d requests ticket lock\n", getpid());
      ticket = ticket_acquire();
      printf(1, "child pid %d ENTERS ticket %d turn %d\n", getpid(), ticket, ticket_turn());
      sleep(20);
      printf(1, "child pid %d releases ticket %d\n", getpid(), ticket);
      ticket_release();
      exit();
    }
  }

  for(i = 0; i < NCHILD; i++)
    wait();

  printf(1, "ticket lock test done\n");
  exit();
}
