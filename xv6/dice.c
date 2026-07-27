#include "types.h"
#include "stat.h"
#include "user.h"

static int
read_roll_count(void)
{
  char buf[16];
  int n;

  printf(1, "number_of_rolls: ");
  n = read(0, buf, sizeof(buf) - 1);
  if(n <= 0)
    return -1;
  buf[n] = 0;
  return atoi(buf);
}

int
main(int argc, char **argv)
{
  int rolls;
  int i;
  int r;
  int values[14];

  if(argc == 2)
    rolls = atoi(argv[1]);
  else if(argc == 1)
    rolls = read_roll_count();
  else{
    printf(2, "usage: dice [rolls]\n");
    exit();
  }

  if(rolls <= 0 || rolls > 14){
    printf(2, "rolls must be in range 1..14\n");
    exit();
  }

  if(setSeed() < 0){
    printf(2, "setSeed failed\n");
    exit();
  }

  if(getRandomNumber(rolls, values) < 0){
    printf(2, "getRandomNumber failed\n");
    exit();
  }

  for(i = 0; i < rolls; i++){
    r = (values[i] % 6) + 1;
    printf(1, "roll %d: %d\n", i + 1, r);
  }

  exit();
}
