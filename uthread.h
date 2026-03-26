#ifndef UTHREAD_H
#define UTHREAD_H

typedef unsigned int uint;

#define MAX_THREADS 8
#define STACK_SIZE  4096

enum tstate {
  T_UNUSED,
  T_RUNNABLE,
  T_RUNNING,
  T_BLOCKED,
  T_ZOMBIE
};

struct context {
  uint eip;
  uint esp;
  uint ebp;
  uint ebx;
  uint esi;
  uint edi;
};

typedef struct thread {
  int tid;
  int state;
  void *stack;
  struct context ctx;
  void (*start_routine)(void *);
  void *arg;
  int waiting_for;
} thread_t;

typedef struct {
  int locked;
  int owner;
} umutex_t;

void thread_switch(struct context *old, struct context *new);

void thread_init(void);
int thread_create(void (*fn)(void*), void *arg);
void thread_yield(void);
int thread_join(int tid);

void mutex_init(umutex_t *m);
void mutex_lock(umutex_t *m);
void mutex_unlock(umutex_t *m);

#endif