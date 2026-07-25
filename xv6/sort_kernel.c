#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
  uint t0, t1;

  if(argc != 2){
    printf(2, "usage: sort_kernel src_file\n");
    exit();
  }

  t0 = uptime();
  if(sort_numbers(argv[1]) < 0){
    printf(2, "sort_numbers failed\n");
    exit();
  }
  t1 = uptime();

  printf(1, "kernel sort done in %d ticks\n", t1 - t0);
  exit();
}
