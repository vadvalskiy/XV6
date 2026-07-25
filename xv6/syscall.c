#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "x86.h"
#include "syscall.h"

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

#define SYSCALL_COUNT_MAX 0x7fffffffU

#if SYSCALL_COUNT_MODE != SYSCALL_COUNT_PERCPU
static volatile uint syscall_count_global[NSYSCALL];
#endif
#if SYSCALL_COUNT_MODE == SYSCALL_COUNT_PERCPU
struct syscall_count_row {
  volatile uint sequence;
  volatile uint count[NSYSCALL];
} __attribute__((aligned(64)));
static struct syscall_count_row syscall_count_cpu[NCPU];
#endif
#if SYSCALL_COUNT_MODE == SYSCALL_COUNT_GLOBAL_LOCKED
static struct spinlock syscall_count_lock;
#endif

void
syscallcountinit(void)
{
#if SYSCALL_COUNT_MODE == SYSCALL_COUNT_GLOBAL_LOCKED
  initlock(&syscall_count_lock, "syscount");
#endif
}

static int
syscall_count_valid(int num)
{
  return num > 0 && num < NSYSCALL;
}

static void
syscall_count_record(int num)
{
  if(!syscall_count_valid(num))
    return;

#if SYSCALL_COUNT_MODE == SYSCALL_COUNT_GLOBAL_UNLOCKED
  if(syscall_count_global[num] < SYSCALL_COUNT_MAX)
    syscall_count_global[num]++;
#elif SYSCALL_COUNT_MODE == SYSCALL_COUNT_GLOBAL_LOCKED
  acquire(&syscall_count_lock);
  if(syscall_count_global[num] < SYSCALL_COUNT_MAX)
    syscall_count_global[num]++;
  release(&syscall_count_lock);
#elif SYSCALL_COUNT_MODE == SYSCALL_COUNT_PERCPU
  {
    int id;
    struct syscall_count_row *row;
    pushcli();
    id = cpuid();
    if(id >= 0 && id < NCPU){
      row = &syscall_count_cpu[id];
      row->sequence++;
      __sync_synchronize();
      if(row->count[num] < SYSCALL_COUNT_MAX)
        row->count[num]++;
      __sync_synchronize();
      row->sequence++;
    }
    popcli();
  }
#else
#error "invalid SYSCALL_COUNT_MODE"
#endif
}

#if SYSCALL_COUNT_MODE == SYSCALL_COUNT_PERCPU
static uint
syscall_count_read_cpu(int cpu_id, int num)
{
  uint before;
  uint after;
  uint value;
  struct syscall_count_row *row = &syscall_count_cpu[cpu_id];

  do{
    before = row->sequence;
    __sync_synchronize();
    value = row->count[num];
    __sync_synchronize();
    after = row->sequence;
  } while((before & 1U) || before != after);
  return value;
}
#endif

int
syscall_count_get(int num)
{
  if(!syscall_count_valid(num))
    return -1;

#if SYSCALL_COUNT_MODE == SYSCALL_COUNT_PERCPU
  {
    int i;
    uint value;
    uint total = 0;
    for(i = 0; i < NCPU; i++){
      value = syscall_count_read_cpu(i, num);
      if(total > SYSCALL_COUNT_MAX - value)
        return (int)SYSCALL_COUNT_MAX;
      total += value;
    }
    return (int)total;
  }
#elif SYSCALL_COUNT_MODE == SYSCALL_COUNT_GLOBAL_LOCKED
  {
    uint total;
    acquire(&syscall_count_lock);
    total = syscall_count_global[num];
    release(&syscall_count_lock);
    return (int)total;
  }
#else
  return (int)syscall_count_global[num];
#endif
}

int
syscall_count_getcpu(int cpu_id, int num)
{
  if(cpu_id < 0 || cpu_id >= NCPU || !syscall_count_valid(num))
    return -1;
#if SYSCALL_COUNT_MODE == SYSCALL_COUNT_PERCPU
  return (int)syscall_count_read_cpu(cpu_id, num);
#else
  // A shared counter has no meaningful per-CPU decomposition.
  return -1;
#endif
}

int
syscall_count_mode(void)
{
  return SYSCALL_COUNT_MODE;
}

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  struct proc *curproc = myproc();

  if(addr >= curproc->sz || addr+4 > curproc->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;
  struct proc *curproc = myproc();

  if(addr >= curproc->sz)
    return -1;
  *pp = (char*)addr;
  ep = (char*)curproc->sz;
  for(s = *pp; s < ep; s++){
    if(*s == 0)
      return s - *pp;
  }
  return -1;
}

// Fetch one byte from user memory at addr.
int
fetchbyte(uint addr, char *cp)
{
  struct proc *curproc = myproc();

  if(addr >= curproc->sz)
    return -1;
  *cp = *(char*)addr;
  return 0;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;
  struct proc *curproc = myproc();
 
  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= curproc->sz || (uint)i+size > curproc->sz)
    return -1;
  *pp = (char*)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_waitstats(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_setSeed(void);
extern int sys_getRandomNumber(void);
extern int sys_process_information(void);
extern int sys_sort_numbers(void);
extern int sys_set_scheduling_info(void);
extern int sys_change_queue(void);
extern int sys_print_scheduling_info(void);
extern int sys_getcount(void);
extern int sys_getcpucount(void);
extern int sys_getcountmode(void);
extern int sys_produce(void);
extern int sys_try_produce(void);
extern int sys_consume(void);
extern int sys_consume_value(void);
extern int sys_try_consume(void);
extern int sys_rw_acquire_read(void);
extern int sys_rw_release_read(void);
extern int sys_rw_acquire_write(void);
extern int sys_rw_release_write(void);
extern int sys_ticket_acquire(void);
extern int sys_ticket_release(void);
extern int sys_ticket_turn(void);

static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_waitstats] sys_waitstats,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_setSeed] sys_setSeed,
[SYS_getRandomNumber] sys_getRandomNumber,
[SYS_process_information] sys_process_information,
[SYS_sort_numbers] sys_sort_numbers,
[SYS_set_scheduling_info] sys_set_scheduling_info,
[SYS_change_queue] sys_change_queue,
[SYS_print_scheduling_info] sys_print_scheduling_info,
[SYS_getcount] sys_getcount,
[SYS_getcpucount] sys_getcpucount,
[SYS_getcountmode] sys_getcountmode,
[SYS_produce] sys_produce,
[SYS_try_produce] sys_try_produce,
[SYS_consume] sys_consume,
[SYS_consume_value] sys_consume_value,
[SYS_try_consume] sys_try_consume,
[SYS_rw_acquire_read] sys_rw_acquire_read,
[SYS_rw_release_read] sys_rw_release_read,
[SYS_rw_acquire_write] sys_rw_acquire_write,
[SYS_rw_release_write] sys_rw_release_write,
[SYS_ticket_acquire] sys_ticket_acquire,
[SYS_ticket_release] sys_ticket_release,
[SYS_ticket_turn] sys_ticket_turn,
};

void
syscall(void)
{
  int num;
  struct proc *curproc = myproc();

  num = curproc->tf->eax;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    syscall_count_record(num);
    curproc->tf->eax = syscalls[num]();
  } else {
    cprintf("%d %s: unknown sys call %d\n",
            curproc->pid, curproc->name, num);
    curproc->tf->eax = -1;
  }
}
