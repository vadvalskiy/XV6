#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

static int
write_file(const char *path, const char *text)
{
  int fd;
  int n = strlen(text);

  unlink(path);
  fd = open(path, O_CREATE | O_WRONLY);
  if(fd < 0)
    return -1;
  if(write(fd, text, n) != n){
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

static int
file_equals(const char *path, const char *expected)
{
  char buf[256];
  int fd;
  int n;
  int total = 0;
  int expected_len = strlen(expected);

  fd = open(path, O_RDONLY);
  if(fd < 0)
    return 0;
  while((n = read(fd, buf + total, sizeof(buf) - total)) > 0){
    total += n;
    if(total == (int)sizeof(buf)){
      close(fd);
      return 0;
    }
  }
  close(fd);
  if(n < 0 || total != expected_len)
    return 0;
  for(n = 0; n < total; n++)
    if(buf[n] != expected[n])
      return 0;
  return 1;
}

static int
run_user_sort(const char *path)
{
  int pid;
  char *argv[] = {"sort_user", (char*)path, 0};

  pid = fork();
  if(pid < 0)
    return -1;
  if(pid == 0){
    exec("sort_user", argv);
    printf(2, "lab2test: exec sort_user failed\n");
    exit();
  }
  return wait() == pid ? 0 : -1;
}

int
main(int argc, char **argv)
{
  const char *input = "3 -1 2147483647 -2147483648 0 3\n";
  const char *expected = "-2147483648\n-1\n0\n3\n3\n2147483647\n";
  int random_values[14];
  int failures = 0;

  if(write_file("lab2-input.txt", input) < 0){
    printf(2, "lab2test: cannot create input\n");
    failures++;
  } else {
    if(sort_numbers("lab2-input.txt") < 0 ||
       !file_equals("lab2-input.txt.kernel.sorted", expected)){
      printf(2, "lab2test: kernel sort regression failed\n");
      failures++;
    }
    if(run_user_sort("lab2-input.txt") < 0 ||
       !file_equals("lab2-input.txt.user.sorted", expected)){
      printf(2, "lab2test: user sort regression failed\n");
      failures++;
    }
  }

  if(write_file("lab2-overflow.txt", "2147483648\n") < 0 ||
     sort_numbers("lab2-overflow.txt") != -1){
    printf(2, "lab2test: integer overflow was not rejected\n");
    failures++;
  }

  if(getRandomNumber(0, random_values) != -1 ||
     getRandomNumber(15, random_values) != -1 ||
     getRandomNumber(14, random_values) < 0){
    printf(2, "lab2test: random syscall validation failed\n");
    failures++;
  }

  if(process_information(getpid()) < 0){
    printf(2, "lab2test: process information syscall failed\n");
    failures++;
  }

  unlink("lab2-input.txt");
  unlink("lab2-input.txt.kernel.sorted");
  unlink("lab2-input.txt.user.sorted");
  unlink("lab2-overflow.txt");
  unlink("lab2-overflow.txt.kernel.sorted");

  if(failures == 0)
    printf(1, "LAB2 TEST PASS\n");
  else
    printf(1, "LAB2 TEST FAIL: %d failure(s)\n", failures);
  exit();
}
