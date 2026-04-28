#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4096


// minimal thread_create
// we allocate memory for a stack then we align it
// it creates a new thread 
int
thread_create(void (*fcn)(void*), void *arg)
{
    void *stack;
    void *base;
    base = malloc(2 * PGSIZE);
    if(base == 0)
        return -1;
    stack = (void*)(((uint)base + PGSIZE - 1) & ~(PGSIZE - 1));
    return clone(fcn, arg, stack);
}

//minimal thread join
int
thread_join(void)
{
    void *stack;
    return join(&stack);
}


// atomic instruction for the lock.
// due to being atomic, multiple threads can safely compete for the same lock 
static inline uint
xchg(uint *addr, uint newval)
{
  uint result;
  asm ("lock; xchgl %0, %1": "+m" (*addr), "=a" (result): "1" (newval): "cc");
  return result;
}

void
lock_init(lock_t *lk)
{
  lk->locked = 0;
}
void
lock_acquire(lock_t *lk)
{
  while(xchg(&lk->locked, 1) != 0)
    ;
}
void
lock_release(lock_t *lk)
{
  xchg(&lk->locked, 0);
}