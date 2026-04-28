#include "types.h"
#include "stat.h"
#include "user.h"
#define FREE 0x0
#define RUNNING 0x1
#define RUNNABLE 0x2
#define STACK_SIZE 8192
#define MAX_THREAD 4
struct thread {
  int sp;
  char stack[STACK_SIZE];
  int state;
};
struct thread all_thread[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(uint32, uint32);
void thread_init(void) {
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}
void thread_yield(void) {
  struct thread *t;
  struct thread *next_thread = 0;
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;
      break;
    }
  }
  if (next_thread == 0) return;
  struct thread *prev_thread = current_thread;
  current_thread = next_thread;
  prev_thread->state = RUNNABLE;
  current_thread->state = RUNNING;
  thread_switch((uint32)&prev_thread->sp, (uint32)&current_thread->sp);
}
void thread_create(void (*func)()) {
  struct thread *t;
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->sp = (int) (t->stack + STACK_SIZE - 8);
  *(int*)(t->sp) = (int)func;
  t->state = RUNNABLE;
}
