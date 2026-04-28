#include "types.h"
#include "stat.h"
#include "user.h"

lock_t temp;
int counter = 0;

void
test(void *arg)
{
  int i;
  for(i = 0; i < 10000; i++){
    lock_acquire(&temp);
    counter++;
    lock_release(&temp);
  }
  exit();
}

int
main(void)
{
  lock_init(&temp);

  thread_create(test, 0);
  thread_create(test, 0);

  thread_join();
  thread_join();

  printf(1, "counter = %d\n", counter);
  exit();
}