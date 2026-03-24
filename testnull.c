#include "types.h"
#include "stat.h"
#include "user.h"

int main(void){
  printf(1, "testnull: about to dereference NULL\n");
  volatile int *p = (int*)0x0;
  volatile int x = *p;
  printf(1, "UNEXPECTED: read=%d\n", x);
  exit();
}
