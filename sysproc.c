#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

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
sys_hello(void){
  cprintf("Hello from Kernel Mode!\n");
  return 0;
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

// int
// sys_sbrk(void)
// {
//   int addr;
//   int n;

//   if(argint(0, &n) < 0)
//     return -1;
//   addr = myproc()->sz;
//   if(growproc(n) < 0)
//     return -1;
//   return addr;
// }

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;  
  myproc()->sz += n;    
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
sys_ticks_running(void)
{
    int pid;
    if(argint(0, &pid) < 0)
        return -1;
    
   return ticks_running(pid);
}

int
sys_sjf_job_length(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;

  return sjf_job_length(pid);
}

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

int
sys_set_sched_priority(void)
{
  int pr;
  if(argint(0, &pr) < 0)
    return -1;

  if(pr < 0 || pr >= PRIORITY)
    return -1;

  struct proc *p = myproc();
  acquire(&ptable.lock);
  p->priority = pr;
  release(&ptable.lock);
  return 0;
}

int
sys_get_sched_priority(void)
{
  int pid;
  if(argint(0, &pid) < 0)
    return -1;

  acquire(&ptable.lock);
  struct proc *q;
  for(q = ptable.proc; q < &ptable.proc[NPROC]; q++){
    if(q->pid == pid){
      int ret = (q->state == RUNNABLE) ? q->priority : -1;
      release(&ptable.lock);
      return ret;
    }
  }
  release(&ptable.lock);
  return -1; // not found
}

int
sys_set_sched_quantum(void)
{
  int q;
  if(argint(0, &q) < 0)
    return -1;

  if(q < MIN_QUANTUM || q > MAX_QUANTUM)
    return -1;

  struct proc *p = myproc();
  acquire(&ptable.lock);
  p->quantum = q;
  // If the process shortens its quantum mid-slice,
  // shift the counter to avoid a long first slice.
  if(p->rrticks > p->quantum)
    p->rrticks = p->quantum - 1;
  release(&ptable.lock);
  return 0;
}
