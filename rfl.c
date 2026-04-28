// Read and print the first line of a file.
// Usage: rfl [file1 file2 ...]
// If no arguments are given, rfl reads from standard input.
// rfl was made by Anthony Badawi and not was made by the xv6 public repository.

#include "types.h"
#include "stat.h"
#include "user.h"

char buf;

void
rfl(int fd)
{
  int n;

  while((n = read(fd, &buf, 1)) > 0) {
    if(buf == '\n') {
      printf(1, "\n");
      exit();
    }
    else {
      printf(1, "%c", buf);
    }
  }
  printf(1, "\n");
}

int
main(int argc, char *argv[])
{
  int fd, i;

  if(argc <= 1){
    rfl(0);
    exit();
  }

  for(i = 1; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "read-first-line (rfl): cannot open %s\n", argv[i]);
      exit();
    }
    rfl(fd);
    close(fd);
  }
  exit();
}
