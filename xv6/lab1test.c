#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

static int
run(char *program, char *argv[])
{
  int pid;

  pid = fork();
  if(pid < 0)
    return -1;
  if(pid == 0){
    exec(program, argv);
    printf(2, "lab1test: exec %s failed\n", program);
    exit();
  }
  if(wait() != pid)
    return -1;
  return 0;
}

static int
file_equals(char *path, char *expected)
{
  char buf[32];
  int fd;
  int n;
  int expected_len;

  fd = open(path, O_RDONLY);
  if(fd < 0)
    return 0;
  n = read(fd, buf, sizeof(buf));
  close(fd);
  expected_len = strlen(expected);
  if(n != expected_len)
    return 0;
  {
    int i;
    for(i = 0; i < expected_len; i++)
      if(buf[i] != expected[i])
        return 0;
  }
  return 1;
}

int
main(void)
{
  char *default_argv[] = {"find_sum", "-o", "lab1-default.txt", "abc12", "7x", "30", 0};
  char *signed_argv[] = {"find_sum", "-o", "lab1-signed.txt", "--signed", "12", "-5", "+7", 0};
  char *overflow_argv[] = {"find_sum", "-o", "lab1-overflow.txt", "999999999999999999", 0};
  int failures;

  failures = 0;
  unlink("lab1-default.txt");
  unlink("lab1-signed.txt");
  unlink("lab1-overflow.txt");

  if(run("find_sum", default_argv) < 0 || !file_equals("lab1-default.txt", "49\n")){
    printf(1, "FAIL lab1 default parsing\n");
    failures++;
  }

  if(run("find_sum", signed_argv) < 0 || !file_equals("lab1-signed.txt", "14\n")){
    printf(1, "FAIL lab1 signed parsing\n");
    failures++;
  }

  run("find_sum", overflow_argv);
  {
    int fd = open("lab1-overflow.txt", O_RDONLY);
    if(fd >= 0){
      close(fd);
      printf(1, "FAIL lab1 overflow rejection\n");
      failures++;
    }
  }

  if(failures == 0)
    printf(1, "LAB1 TEST PASS\n");
  else
    printf(1, "LAB1 TEST FAIL: %d failure(s)\n", failures);
  exit();
}
