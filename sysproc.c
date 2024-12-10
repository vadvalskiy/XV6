#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;
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
sys_setburst(void)
{
    int burst;

    if(argint(0, &burst) < 0 || burst <= 0)
        return -1;  // Invalid burst time
    myproc()->burst_time = burst;
    return 0;
}

int
sys_setconfidence(void)
{
    int conf;

    if(argint(0, &conf) < 0 || conf < 0 || conf > 99)
        return -1;  // Confidence must be in the range 0-99
    myproc()->confidence = conf;
    return 0;
}

int sys_setpriority(void) {
    int prio;
    if (argint(0, &prio) < 0 || prio < 1 || prio > 3)
        return -1;
    myproc()->priority = prio;
    return 0;
}

int sys_printinfo(void)
{
  acquire(&ptable.lock);
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if (p->pid > 0) {
        static char* state_names[] = { [UNUSED] "UNUSED" , [EMBRYO] "EMBRYO" , [SLEEPING] "SLEEPING" , [RUNNABLE] "RUNNABLE" , [RUNNING] "RUNNING" , [ZOMBIE] "ZOMBIE"};
        cprintf("----------------------\n");
        cprintf("name: %s - ",p->name);
        cprintf("pid: %d - ",p->pid);
        cprintf("state: %s - ",state_names[p->state]);
        cprintf("burst time: %d - ",p->burst_time);
        cprintf("confidence: %d - ",p->confidence);
        cprintf("create time: %d - ",p->creation_order);
        cprintf("priority: %d - ",p->priority);
        cprintf("last run time: %d\n",p->last_run_time);
        cprintf("-----------------------\n");
      }
  }
  release(&ptable.lock);
  return 0;
}
