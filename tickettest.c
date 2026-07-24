#include "types.h"
#include "stat.h"
#include "user.h"

#define NCHILD 8

int
main(int argc, char *argv[])
{
  int p[2], recovery[2];
  int values[NCHILD];
  int i, pid, ticket, first, second;
  int failures = 0;

  for(i = 0; i < NCHILD; i++) values[i] = -1;
  if(ticket_release() != -1) failures++;
  if(pipe(p) < 0) exit();
  for(i = 0; i < NCHILD; i++){
    pid = fork();
    if(pid == 0){
      close(p[0]);
      sleep(i);
      ticket = ticket_acquire();
      if(ticket < 0) exit();
      write(p[1], &ticket, sizeof(ticket));
      sleep(3);
      if(ticket_release() < 0) exit();
      close(p[1]);
      exit();
    }
  }
  close(p[1]);
  for(i = 0; i < NCHILD; i++) wait();
  for(i = 0; i < NCHILD; i++)
    if(read(p[0], &values[i], sizeof(values[i])) != sizeof(values[i])) failures++;
  close(p[0]);
  for(i = 1; i < NCHILD; i++) if(values[i] != values[0] + i) failures++;

  // Verify exit cleanup: the first child abandons the lock; the next request
  // must still make progress and receive the following ticket.
  if(pipe(recovery) < 0) exit();
  pid = fork();
  if(pid == 0){
    close(recovery[0]);
    ticket = ticket_acquire();
    write(recovery[1], &ticket, sizeof(ticket));
    exit();
  }
  wait();
  pid = fork();
  if(pid == 0){
    close(recovery[0]);
    ticket = ticket_acquire();
    write(recovery[1], &ticket, sizeof(ticket));
    ticket_release();
    exit();
  }
  close(recovery[1]);
  wait();
  if(read(recovery[0], &first, sizeof(first)) != sizeof(first) ||
     read(recovery[0], &second, sizeof(second)) != sizeof(second) ||
     second != first + 1) failures++;
  close(recovery[0]);

  if(failures == 0)
    printf(1, "LAB4 TICKET TEST PASS\n");
  else
    printf(1, "LAB4 TICKET TEST FAIL: %d failure(s)\n", failures);
  exit();
}
