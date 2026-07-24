
#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "fcntl.h"
#include "file.h"

extern uint ticks;
extern struct spinlock tickslock;

static struct spinlock randlock;
static int randinit;
static uint random_state;

static void
randominit_once(void)
{
  acquire(&tickslock);
  if(randinit == 0){
    initlock(&randlock, "randlock");
    randinit = 1;
  }
  release(&tickslock);
}

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
sys_setSeed(void)
{
  uint now_ticks;
  uint pid;
  uint stack_addr;
  uint seed;
  int local_stack_var;

  randominit_once();

  acquire(&tickslock);
  now_ticks = ticks;
  release(&tickslock);

  pid = myproc()->pid;
  stack_addr = (uint)&local_stack_var;
  seed = (now_ticks << 20) ^ (pid << 10) ^ (stack_addr & 0x3FF);

  acquire(&randlock);
  random_state = seed;
  release(&randlock);

  return 0;
}

int
sys_getRandomNumber(void)
{
  int n;
  int *ubuf;
  int i;
  uint value;

  randominit_once();

  if(argint(0, &n) < 0)
    return -1;
  if(n <= 0 || n >= 15)
    return -1;
  if(argptr(1, (char**)&ubuf, n * sizeof(int)) < 0)
    return -1;

  acquire(&randlock);
  for(i = 0; i < n; i++){
    random_state = (1103515245U * random_state + 12345U) & 0x7fffffffU;
    value = random_state;
    if(copyout(myproc()->pgdir, (uint)&ubuf[i], &value, sizeof(value)) < 0){
      release(&randlock);
      return -1;
    }
  }
  release(&randlock);

  return 0;
}

int
sys_process_information(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return procinfo(pid);
}


