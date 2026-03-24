#include "types.h"
#include "stat.h"
#include "user.h"

int main(void){
  printf(1, "testkaccess: illegal read attempt\n");
  volatile int *p = (int*)0xFFFF0000;
  volatile int x = *p;
  printf(1, "UNEXPECTED: read=%d\n", x);
  exit();
}
