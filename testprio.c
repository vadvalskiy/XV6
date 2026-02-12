#include "types.h"
#include "stat.h"
#include "user.h"

static void check(const char *msg, int cond) {
  if(cond) printf(1, "PASS: %s\n", msg);
  else { printf(1, "FAIL: %s\n", msg); exit(); }
}

int main(void){
  int me = getpid();
  int r;

  r = setpriority(me, 1);
  check("setpriority(self,1) returns 0", r == 0);

  r = setpriority(-1, 1);
  check("setpriority(-1,1) returns -1", r == -1);

  r = setpriority(me, 99);
  check("setpriority(self,99) returns -1", r == -1);

  printf(1, "testprio: done\n");
  exit();
}
