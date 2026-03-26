#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

static thread_t threads[MAX_THREADS];
static int current_tid = 0;
static int initialized = 0;

static void thread_stub(void);
static void schedule(void);
static int find_next_runnable(void);
static void wake_joiners(int finished_tid);

void
thread_init(void)
{
  int i;

  if(initialized)
    return;

  for(i = 0; i < MAX_THREADS; i++){
    threads[i].tid = i;
    threads[i].state = T_UNUSED;
    threads[i].stack = 0;
    threads[i].start_routine = 0;
    threads[i].arg = 0;
    threads[i].waiting_for = -1;

    threads[i].ctx.eip = 0;
    threads[i].ctx.esp = 0;
    threads[i].ctx.ebp = 0;
    threads[i].ctx.ebx = 0;
    threads[i].ctx.esi = 0;
    threads[i].ctx.edi = 0;
  }

  current_tid = 0;
  threads[0].state = T_RUNNING;
  threads[0].stack = 0;

  initialized = 1;
}

static int
find_next_runnable(void)
{
  int i;

  for(i = 1; i <= MAX_THREADS; i++){
    int idx = (current_tid + i) % MAX_THREADS;
    if(threads[idx].state == T_RUNNABLE)
      return idx;
  }

  return -1;
}

static void
schedule(void)
{
  int prev, next;

  prev = current_tid;
  next = find_next_runnable();

  if(next < 0)
    return;

  if(threads[prev].state == T_RUNNING)
    threads[prev].state = T_RUNNABLE;

  threads[next].state = T_RUNNING;
  current_tid = next;

  thread_switch(&threads[prev].ctx, &threads[next].ctx);
}

void
thread_yield(void)
{
  schedule();
}

static void
wake_joiners(int finished_tid)
{
  int i;

  for(i = 0; i < MAX_THREADS; i++){
    if(threads[i].state == T_BLOCKED && threads[i].waiting_for == finished_tid){
      threads[i].waiting_for = -1;
      threads[i].state = T_RUNNABLE;
    }
  }
}

static void
thread_stub(void)
{
  int tid = current_tid;

  if(threads[tid].start_routine != 0)
    threads[tid].start_routine(threads[tid].arg);

  threads[tid].state = T_ZOMBIE;
  wake_joiners(tid);

  thread_yield();

  exit();
}

int
thread_create(void (*fn)(void*), void *arg)
{
  int i;
  char *stack;
  uint stack_top;

  if(!initialized)
    thread_init();

  for(i = 0; i < MAX_THREADS; i++){
    if(threads[i].state == T_UNUSED)
      break;
  }

  if(i == MAX_THREADS)
    return -1;

  stack = malloc(STACK_SIZE);
  if(stack == 0)
    return -1;

  stack_top = (uint)stack + STACK_SIZE;

  threads[i].tid = i;
  threads[i].state = T_RUNNABLE;
  threads[i].stack = stack;
  threads[i].start_routine = fn;
  threads[i].arg = arg;
  threads[i].waiting_for = -1;

  threads[i].ctx.eip = (uint)thread_stub;
  threads[i].ctx.esp = stack_top;
  threads[i].ctx.ebp = stack_top;
  threads[i].ctx.ebx = 0;
  threads[i].ctx.esi = 0;
  threads[i].ctx.edi = 0;

  return i;
}

int
thread_join(int tid)
{
  if(tid < 0 || tid >= MAX_THREADS)
    return -1;

  if(tid == current_tid)
    return -1;

  if(threads[tid].state == T_UNUSED)
    return -1;

  while(threads[tid].state != T_ZOMBIE){
    threads[current_tid].waiting_for = tid;
    threads[current_tid].state = T_BLOCKED;
    schedule();
  }

  if(threads[tid].stack != 0){
    free(threads[tid].stack);
    threads[tid].stack = 0;
  }

  threads[tid].state = T_UNUSED;
  threads[tid].start_routine = 0;
  threads[tid].arg = 0;
  threads[tid].waiting_for = -1;

  threads[tid].ctx.eip = 0;
  threads[tid].ctx.esp = 0;
  threads[tid].ctx.ebp = 0;
  threads[tid].ctx.ebx = 0;
  threads[tid].ctx.esi = 0;
  threads[tid].ctx.edi = 0;

  return 0;
}

void
mutex_init(umutex_t *m)
{
  m->locked = 0;
  m->owner = -1;
}

void
mutex_lock(umutex_t *m)
{
  while(m->locked){
    thread_yield();
  }

  m->locked = 1;
  m->owner = current_tid;
}

void
mutex_unlock(umutex_t *m)
{
  if(m->locked && m->owner == current_tid){
    m->locked = 0;
    m->owner = -1;
  }
}