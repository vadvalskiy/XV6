//
// find_sum: scan command-line text, sum decimal integer sequences, and write
// the result to a file.  The default mode preserves the assignment behavior
// (all digit sequences are non-negative); --signed enables leading +/- signs.
//

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define INT_MAX_VALUE 2147483647
#define INT_MIN_VALUE (-2147483647 - 1)
#define INT_MIN_MAGNITUDE 2147483648U
#define OUTPUT_PATH_MAX 128

static int
is_digit(char c)
{
  return c >= '0' && c <= '9';
}

static int
checked_add(int a, int b, int *out)
{
  if(b > 0 && a > INT_MAX_VALUE - b)
    return -1;
  if(b < 0 && a < INT_MIN_VALUE - b)
    return -1;
  *out = a + b;
  return 0;
}

static int
parse_magnitude(char *s, int *index, int sign, int *value)
{
  uint magnitude;
  uint limit;
  int digit;

  magnitude = 0;
  limit = sign < 0 ? INT_MIN_MAGNITUDE : (uint)INT_MAX_VALUE;

  while(is_digit(s[*index])){
    digit = s[*index] - '0';
    if(magnitude > (limit - (uint)digit) / 10U)
      return -1;
    magnitude = magnitude * 10U + (uint)digit;
    (*index)++;
  }

  if(sign < 0){
    if(magnitude == INT_MIN_MAGNITUDE)
      *value = INT_MIN_VALUE;
    else
      *value = -(int)magnitude;
  } else {
    *value = (int)magnitude;
  }
  return 0;
}

static int
parse_args_sum(int argc, char *argv[], int first, int signed_mode, int *sum_out)
{
  int i;
  int sum;

  sum = 0;
  for(i = first; i < argc; i++){
    char *s;
    int j;

    s = argv[i];
    j = 0;
    while(s[j]){
      int sign;
      int value;
      int starts_number;

      sign = 1;
      starts_number = is_digit(s[j]);
      if(signed_mode && (s[j] == '-' || s[j] == '+') && is_digit(s[j + 1])){
        sign = s[j] == '-' ? -1 : 1;
        j++;
        starts_number = 1;
      }

      if(!starts_number){
        j++;
        continue;
      }

      if(parse_magnitude(s, &j, sign, &value) < 0)
        return -1;
      if(checked_add(sum, value, &sum) < 0)
        return -1;
    }
  }

  *sum_out = sum;
  return 0;
}

static int
int_to_buf(int value, char *buf, int bufsz)
{
  char reverse[16];
  uint magnitude;
  int n;
  int r;

  if(bufsz < 3)
    return -1;

  n = 0;
  if(value < 0){
    buf[n++] = '-';
    magnitude = (uint)(-(value + 1)) + 1U;
  } else {
    magnitude = (uint)value;
  }

  r = 0;
  do{
    if(r >= (int)sizeof(reverse))
      return -1;
    reverse[r++] = '0' + (magnitude % 10U);
    magnitude /= 10U;
  } while(magnitude > 0);

  if(n + r + 1 > bufsz)
    return -1;
  while(r > 0)
    buf[n++] = reverse[--r];
  buf[n++] = '\n';
  return n;
}

static int
write_result(const char *path, char *buf, int len)
{
  int fd;
  int written;

  // xv6 O_CREATE does not truncate an existing file.
  unlink(path);
  fd = open(path, O_CREATE | O_WRONLY);
  if(fd < 0){
    printf(2, "find_sum: cannot open %s\n", path);
    return -1;
  }

  written = write(fd, buf, len);
  close(fd);
  if(written != len){
    unlink(path);
    printf(2, "find_sum: write failed\n");
    return -1;
  }
  return 0;
}

static void
usage(void)
{
  printf(2, "usage: find_sum [-o output] [--signed] text...\n");
}

int
main(int argc, char *argv[])
{
  char *output;
  char buf[32];
  int signed_mode;
  int first;
  int sum;
  int len;

  output = "result.txt";
  signed_mode = 0;
  first = 1;

  while(first < argc){
    if(strcmp(argv[first], "--signed") == 0){
      signed_mode = 1;
      first++;
    } else if(strcmp(argv[first], "-o") == 0){
      if(first + 1 >= argc){
        usage();
        exit();
      }
      output = argv[first + 1];
      if(strlen(output) == 0 || strlen(output) >= OUTPUT_PATH_MAX){
        printf(2, "find_sum: invalid output path\n");
        exit();
      }
      first += 2;
    } else if(strcmp(argv[first], "--") == 0){
      first++;
      break;
    } else {
      break;
    }
  }

  if(parse_args_sum(argc, argv, first, signed_mode, &sum) < 0){
    printf(2, "find_sum: integer overflow\n");
    unlink(output);
    exit();
  }

  len = int_to_buf(sum, buf, sizeof(buf));
  if(len < 0 || write_result(output, buf, len) < 0)
    exit();

  printf(1, "find_sum: wrote %d to %s\n", sum, output);
  exit();
}
