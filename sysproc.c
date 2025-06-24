#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_thread_create(void)
{
  void (*func) (void);
  void *tstack;
  int stacksize;
  void (*wrapper) (uint);

  if(argptr(0, (void*)&func, sizeof(void*)) < 0 || argptr(1, (void*)&tstack, sizeof(void*)) < 0 || argint(2, &stacksize) < 0 || argptr(3, (void*)&wrapper, sizeof(void*)) < 0)
    return -1;

  return thread_create(func, tstack, stacksize, wrapper);
}

int
sys_thread_exit(void)
{
  void *ret;

  if(argptr(0, (void*)&ret, sizeof(void*)) < 0)
    return -1;

  thread_exit(ret);

  return 0; // not reached
}

int
sys_thread_join(void)
{
  int tid;
  void **retval;

  if(argint(0, &tid) < 0 || argptr(1, (void*)&retval, sizeof(void*)) < 0)
    return -1;

  return thread_join(tid, retval);
}

int
sys_gettid(void)
{
  return gettid();
}