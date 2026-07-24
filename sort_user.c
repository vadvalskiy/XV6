#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAX_NUMS 1024
#define READ_BUFSZ 128

static int nums[MAX_NUMS];
static char readbuf[READ_BUFSZ];
static char outpath[128];
static char line[32];

static int
is_space(char c)
{
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static void
sort_ints(int *a, int n)
{
  int i;

  for(i = 1; i < n; i++){
    int key = a[i];
    int j = i - 1;
    while(j >= 0 && a[j] > key){
      a[j + 1] = a[j];
      j--;
    }
    a[j + 1] = key;
  }
}

static int
append_suffix(const char *src, char *dst, int dstsz, const char *suf)
{
  int i = 0;
  int j = 0;

  while(src[i] && i < dstsz - 1){
    dst[i] = src[i];
    i++;
  }
  if(src[i] != 0)
    return -1;
  while(suf[j] && i < dstsz - 1)
    dst[i++] = suf[j++];
  if(suf[j] != 0)
    return -1;
  dst[i] = 0;
  return 0;
}

static int
int_to_line(int v, char *dst, int dstsz)
{
  char tmp[16];
  uint x;
  int n = 0;
  int i = 0;

  if(v < 0){
    if(dstsz < 2)
      return -1;
    dst[n++] = '-';
    x = (uint)(-(v + 1)) + 1;
  } else {
    x = (uint)v;
  }

  do{
    if(i >= (int)sizeof(tmp))
      return -1;
    tmp[i++] = '0' + (x % 10);
    x /= 10;
  } while(x > 0);

  if(n + i + 1 >= dstsz)
    return -1;

  while(i > 0)
    dst[n++] = tmp[--i];
  dst[n++] = '\n';
  return n;
}

static int
read_numbers(int fd, int *out, int maxn)
{
  int count = 0;
  int nread;
  int i;
  int in_number = 0;
  int seen_digit = 0;
  int sign = 1;
  int value = 0;

  while((nread = read(fd, readbuf, sizeof(readbuf))) > 0){
    for(i = 0; i < nread; i++){
      char ch = readbuf[i];

      if(is_space(ch)){
        if(in_number){
          if(!seen_digit || count >= maxn)
            return -1;
          out[count++] = sign * value;
          in_number = 0;
          seen_digit = 0;
          sign = 1;
          value = 0;
        }
        continue;
      }

      if(!in_number){
        in_number = 1;
        seen_digit = 0;
        sign = 1;
        value = 0;
        if(ch == '-'){
          sign = -1;
          continue;
        }
        if(ch == '+')
          continue;
      }

      if(ch < '0' || ch > '9')
        return -1;
      seen_digit = 1;
      value = value * 10 + (ch - '0');
    }
  }
  if(nread < 0)
    return -1;

  if(in_number){
    if(!seen_digit || count >= maxn)
      return -1;
    out[count++] = sign * value;
  }

  return count;
}

int
main(int argc, char **argv)
{
  int fdin = -1;
  int fdout = -1;
  int count;
  int i;
  int n;
  uint t0, t1;

  if(argc != 2){
    printf(2, "usage: sort_user src_file\n");
    exit();
  }

  fdin = open(argv[1], O_RDONLY);
  if(fdin < 0){
    printf(2, "cannot open input file\n");
    exit();
  }

  count = read_numbers(fdin, nums, MAX_NUMS);
  close(fdin);
  if(count < 0){
    printf(2, "parse/read failed\n");
    exit();
  }

  if(append_suffix(argv[1], outpath, sizeof(outpath), ".user.sorted") < 0){
    printf(2, "output path too long\n");
    exit();
  }

  t0 = uptime();
  sort_ints(nums, count);
  t1 = uptime();

  unlink(outpath); // open(O_CREATE) in xv6 does not truncate existing files.
  fdout = open(outpath, O_CREATE | O_WRONLY);
  if(fdout < 0){
    printf(2, "cannot open output file\n");
    exit();
  }

  for(i = 0; i < count; i++){
    n = int_to_line(nums[i], line, sizeof(line));
    if(n <= 0 || write(fdout, line, n) != n){
      printf(2, "write failed\n");
      close(fdout);
      exit();
    }
  }
  close(fdout);

  printf(1, "user sort: %d numbers sorted into %s in %d ticks\n", count, outpath, t1 - t0);
  exit();
}
