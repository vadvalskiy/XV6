#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  int r;

  printf(1, "my pid = %d\n", pid);

  r = setpriority(pid, 1);
  printf(1, "setpriority(pid, 1) -> %d (expect 0)\n", r);

  r = setpriority(pid, 7);
  printf(1, "setpriority(pid, 7) -> %d (expect -1)\n", r);

  r = setpriority(999999, 1);
  printf(1, "setpriority(999999, 1) -> %d (expect -1)\n", r);

  exit();
}
