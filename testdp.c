#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  int pages;
  char *buf;
  int i, pid, s;

  printf(1, "test: demand paging\n");
  pages = 1500;
  buf = sbrk(pages * 4096);
  for(i = 0; i < pages; i += 128){
    buf[i * 4096] = 'A';
  }
  printf(1, "demand paging: OK\n");

  printf(1, "test: eviction + swapin\n");
  pages = 70000;
  buf = sbrk(pages * 4096);
  for(i = 0; i < pages; i++){
    buf[i * 4096] = i;
  }
  s = 0;
  for(i = 0; i < pages; i++){
    s += buf[i * 4096];
  }
  printf(1, "sum = %d\n", s);
  
  printf(1, "all tests passed\n");

  exit();
}

