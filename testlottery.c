#include "types.h"
#include "stat.h"
#include "user.h"

static volatile int sink = 0;
static void burn(int n){ for(int i=0;i<n;i++) sink += i; }

int main(int argc, char **argv){
  int t = 10;
  if(argc == 2) t = atoi(argv[1]);

  if(settickets(0) == 0){
    printf(1, "FAIL: settickets(0) should fail\n");
    exit();
  }
  if(t >= 1 && settickets(t) != 0){
    printf(1, "FAIL: settickets(%d) should succeed\n", t);
    exit();
  }
  printf(1, "PASS: settickets validation\n");

  for(int k=0;k<200;k++) burn(200000);
  printf(1, "testlottery: done\n");
  exit();
}
