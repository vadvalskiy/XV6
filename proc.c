#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

int
getnextpid(void)
{
  static int nextpid = MIN_PID;

  if(nextpid == MAX_PID)
    nextpid = MIN_PID;

  return nextpid++;
}

int
getnexttid(void)
{
  static int nexttid = MIN_TID;

  if(nexttid == MAX_TID)
    nexttid = MIN_TID;

  return nexttid++;
}

static struct proc*
findavailableproc(int isthread)
{
  struct proc *p, *availableproc = 0;
  int i;
  int availablepid;
  int availabletid = 0;

  if(isthread)
    availablepid = myproc()->pid;
  else
    availablepid = 0;

  acquire(&ptable.lock);

  // loop until we find available pid&tid.
  // logically it should take less than NPROC loops because NPROC < MAX_PID/TID, if it doesn't treat it as an error
  for(i = 0; i < NPROC && (!availablepid || !availabletid); i++){
    availabletid = getnexttid();
    if(!isthread && !availablepid)
      availablepid = getnextpid();

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != UNUSED){
        if(availabletid == p->tid)
          availabletid = 0;
        if(!isthread && availablepid == p->pid)
          availablepid = 0;
      } else if(!availableproc){
        availableproc = p;
      }
    }

    // if not found an unused proc in the first loop, no need to keep looking
    if(!availableproc)
      goto error;
  }

  if(!availablepid || !availabletid)
    goto error;

  availableproc->state = EMBRYO;
  availableproc->pid = availablepid;
  availableproc->tid = availabletid;

  release(&ptable.lock);

  return availableproc;

error:
  release(&ptable.lock);
  return 0;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
newproc(int isthread)
{
  struct proc *p;
  char *sp;

  p = findavailableproc(isthread);
  if(p == 0)
    return 0;

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

static struct proc*
allocproc(void)
{
  return newproc(0);
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

int
clone(int isthread, void (*func) (void), void *tstack, int stacksize, void (*wrapper) (uint))
{
  int i;
  // uint ustack[3];
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = newproc(isthread)) == 0){
    return -1;
  }

  // Copy process state from proc.
  if(isthread)
    np->pgdir = curproc->pgdir;
  else{
    if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
      kfree(np->kstack);
      np->kstack = 0;
      np->state = UNUSED;
      return -1;
    }
  }
  np->sz = curproc->sz;
  *np->tf = *curproc->tf;
  
  if(isthread){
    np->parent = curproc->parent;

    // Set it to jump to `func` when returing to the userspace.
    np->tf->eip = (uint)wrapper;
    np->tf->esp = (uint)tstack + stacksize;

    np->tf->esp -= 4;
    *(uint*)(np->tf->esp) = (uint)func;
    np->tf->esp -= 4;
    *(uint*)(np->tf->esp) = 0xafffffff;
    // if(copyout(np->pgdir, np->tf->esp, ustack, 8) < 0)
    //   return -1;
  }
  else{
    np->parent = curproc;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;
  }

  np->joined = 0;

  for(i = 0; i < NOFILE; i++){
    if(curproc->ofile[i]){
      if(isthread)
        np->ofile[i] = curproc->ofile[i];
      else
        np->ofile[i] = filedup(curproc->ofile[i]);
    } else{
      np->ofile[i] = 0;
    }
  }

  if(isthread)
    np->cwd = curproc->cwd;
  else
    np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  
  release(&ptable.lock);
  
  if (isthread)
    return np->tid;

  return np->pid;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  return clone(0, 0, 0, 0, 0);
}

int
thread_create(void (*func) (void), void *tstack, int stacksize, void (*wrapper) (uint))
{
  if(!func || !tstack || !stacksize || !wrapper)
    return -1;

  return clone(1, func, tstack, stacksize, wrapper);
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  changecwd(0);

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == curproc->pid){
      p->state = ZOMBIE;
    } else if(p->parent->pid == curproc->pid){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

void thread_exit(void *retval)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int islastofpid = 1;

  if(curproc == initproc)
    panic("init exiting");

  // cprintf("%s:%d\n", __FILE__, __LINE__);

  acquire(&ptable.lock);

  // Check if this is the last thread of the group
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p != curproc && p->pid == curproc->pid){
      islastofpid = 0;
      break;
    }
  }

  // cprintf("%s:%d\n", __FILE__, __LINE__);

  release(&ptable.lock);

  if(islastofpid){
    exit();
    // doesn't reach here
  }

  // cprintf("%s:%d\n", __FILE__, __LINE__);

  curproc->retval = retval;

  acquire(&ptable.lock);

  // Another thread might be sleeping in thread_join().
  if(curproc->joined)
    wakeup1(curproc->joined);

  // cprintf("%s:%d\n", __FILE__, __LINE__);

  // Jump into the scheduler, never to return.
  curproc->state = THREAD_ZOMBIE;
  sched();
  panic("zombie exit");
}

void
freeproc(struct proc *p)
{
  kfree(p->kstack);
  p->kstack = 0;
  p->pid = 0;
  p->tid = 0;
  p->joined = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->killed = 0;
  p->state = UNUSED;
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid = 0;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      // If we didn't find a pid yet, skip if p is not a child of curproc. If we already found a child pid, skip all others, even if they are children, in order for `wait` to be a per-pid call.
      if((!pid && p->parent->pid != curproc->pid) || (pid && p->pid != pid))
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        // We need to continue to search for other procs with the same pid (aka threads), so if this is the first time free the adress space as it is shared across threads.
        if(!pid){
          pid = p->pid;
          freevm(p->pgdir);
        }
        freeproc(p);
      }
    }

    if(pid){
      release(&ptable.lock);
      return pid;
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int
thread_join(int tid, void **retval)
{
  struct proc *p;
  int foundthread = 0;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for 
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->tid != tid)
        continue;
      
      if(p->pid != curproc->pid){
        // The calling process tried to join a thread that is not part of his "group" 
        release(&ptable.lock);
        return -1;
      }

      if(!foundthread){
        if(p->joined){
          // The target thread is already joined
          release(&ptable.lock);
          return -2;
        }
        
        p->joined = curproc;
      }
      
      foundthread = 1;
      if(p->state == THREAD_ZOMBIE){
        // Found one.
        if(retval)
          *retval = p->retval;
        freeproc(p);
        release(&ptable.lock);
        return tid;
      }
    }
    
    // No point waiting if the thread doesn't exist.
    if(!foundthread){
      // If we reached here then we didn't set joined to us, so no need to clean it
      release(&ptable.lock);
      return -1;
    }
    
    // Wait for the thread to exit.  (See wakeup1 call in exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int
gettid()
{
  return myproc()->tid;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;
  int found = 0;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      found = 1;
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
    }
  }
  release(&ptable.lock);

  if(found)
    return 0;
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// Sets all the cwd of all processes part of the current process's group.
// Assumes that ptable is not already locked by us.
void
changecwd(struct inode *ip)
{
  struct proc *p;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->pid == curproc->pid)
      p->cwd = ip;

  release(&ptable.lock);
}