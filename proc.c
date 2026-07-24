#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "schedstat.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static char *pstate_names[] = {
[UNUSED]    "UNUSED",
[EMBRYO]    "EMBRYO",
[SLEEPING]  "SLEEPING",
[RUNNABLE]  "RUNNABLE",
[RUNNING]   "RUNNING",
[ZOMBIE]    "ZOMBIE"
};

static struct proc *initproc;
extern uint ticks;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);


static uint sched_rand_state = 0x12345678;

static int
sched_queue_quota(int q)
{
  if(q == SCHED_Q_RR)
    return 3 * SCHED_WRR_UNIT_TICKS;
  if(q == SCHED_Q_SJF)
    return 2 * SCHED_WRR_UNIT_TICKS;
  return SCHED_WRR_UNIT_TICKS;
}

static int
sched_valid_queue(int q)
{
  return q >= 0 && q < SCHED_NQUEUE;
}

static int
sched_rand100(void)
{
  sched_rand_state = sched_rand_state * 1103515245U + 12345U + ticks;
  return (sched_rand_state >> 16) % 100;
}

static void
sched_init_proc_fields(struct proc *p)
{
  p->sched_queue = SCHED_Q_FCFS;
  p->sched_wait_ticks = 0;
  p->sched_burst_time = 2;
  p->sched_confidence = 50;
  p->sched_consecutive_ticks = 0;
  p->sched_arrival_tick = ticks;
  p->sched_shell_path = 0;
  p->sched_created_tick = ticks;
  p->sched_first_run_tick = ~0U;
  p->sched_exit_tick = 0;
  p->sched_runtime_ticks = 0;
  p->sched_runnable_ticks = 0;
  p->sched_dispatches = 0;
  p->sched_preemptions = 0;
  p->lab4_read_lock_held = 0;
  p->lab4_write_lock_held = 0;
  p->lab4_ticket_waiting = 0;
  p->lab4_ticket_held = 0;
  p->lab4_ticket_number = 0;
}

static int
sched_queue_has_runnable(int q)
{
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == RUNNABLE && p->sched_queue == q)
      return 1;
  return 0;
}

static struct proc*
sched_pick_rr(struct cpu *c)
{
  int i, idx;
  struct proc *p;

  for(i = 0; i < NPROC; i++){
    idx = (c->sched_rr_next + i) % NPROC;
    p = &ptable.proc[idx];
    if(p->state == RUNNABLE && p->sched_queue == SCHED_Q_RR){
      c->sched_rr_next = (idx + 1) % NPROC;
      return p;
    }
  }
  return 0;
}

static int
sched_less_sjf(struct proc *a, struct proc *b)
{
  if(b == 0)
    return 1;
  if(a->sched_burst_time != b->sched_burst_time)
    return a->sched_burst_time < b->sched_burst_time;
  if(a->sched_arrival_tick != b->sched_arrival_tick)
    return a->sched_arrival_tick < b->sched_arrival_tick;
  return a->pid < b->pid;
}

static int
sched_after_sjf_key(struct proc *p, int last_burst, uint last_arrival, int last_pid)
{
  if(last_pid < 0)
    return 1;
  if(p->sched_burst_time != last_burst)
    return p->sched_burst_time > last_burst;
  if(p->sched_arrival_tick != last_arrival)
    return p->sched_arrival_tick > last_arrival;
  return p->pid > last_pid;
}

static int
sched_count_queue(int q)
{
  struct proc *p;
  int n = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == RUNNABLE && p->sched_queue == q)
      n++;
  return n;
}

static struct proc*
sched_pick_sjf(void)
{
  int total, step, last_burst, last_pid;
  uint last_arrival;
  struct proc *p, *best;

  total = sched_count_queue(SCHED_Q_SJF);
  if(total == 0)
    return 0;

  last_burst = 0;
  last_arrival = 0;
  last_pid = -1;
  for(step = 0; step < total; step++){
    best = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE || p->sched_queue != SCHED_Q_SJF)
        continue;
      if(!sched_after_sjf_key(p, last_burst, last_arrival, last_pid))
        continue;
      if(sched_less_sjf(p, best))
        best = p;
    }
    if(best == 0)
      return 0;
    if(step == total - 1 || sched_rand100() <= best->sched_confidence)
      return best;
    last_burst = best->sched_burst_time;
    last_arrival = best->sched_arrival_tick;
    last_pid = best->pid;
  }
  return 0;
}

static struct proc*
sched_pick_fcfs(void)
{
  struct proc *p, *best = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->sched_queue != SCHED_Q_FCFS)
      continue;
    if(best == 0 || p->sched_arrival_tick < best->sched_arrival_tick ||
       (p->sched_arrival_tick == best->sched_arrival_tick && p->pid < best->pid))
      best = p;
  }
  return best;
}

static struct proc*
sched_pick_from_queue(struct cpu *c, int q)
{
  if(q == SCHED_Q_RR)
    return sched_pick_rr(c);
  if(q == SCHED_Q_SJF)
    return sched_pick_sjf();
  if(q == SCHED_Q_FCFS)
    return sched_pick_fcfs();
  return 0;
}

static void
sched_advance_queue(struct cpu *c)
{
  c->sched_queue = (c->sched_queue + 1) % SCHED_NQUEUE;
  c->sched_queue_ticks = 0;
}

static int
sched_queue_of_new_child(struct proc *parent)
{
  if(parent == initproc || parent->sched_shell_path)
    return SCHED_Q_RR;
  return SCHED_Q_FCFS;
}

static void
sched_mark_runnable_after_sleep(struct proc *p)
{
  p->state = RUNNABLE;
  p->sched_wait_ticks = 0;
  p->sched_consecutive_ticks = 0;
  p->sched_arrival_tick = ticks;
}

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

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  sched_init_proc_fields(p);

  release(&ptable.lock);

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

  p->sched_queue = SCHED_Q_RR;
  p->sched_shell_path = 1;
  p->sched_arrival_tick = ticks;
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

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->sched_queue = sched_queue_of_new_child(curproc);
  np->sched_shell_path = curproc->sched_shell_path;
  np->sched_wait_ticks = 0;
  np->sched_consecutive_ticks = 0;
  np->sched_arrival_tick = ticks;
  np->sched_created_tick = ticks;
  np->sched_first_run_tick = ~0U;
  np->sched_exit_tick = 0;
  np->sched_runtime_ticks = 0;
  np->sched_runnable_ticks = 0;
  np->sched_dispatches = 0;
  np->sched_preemptions = 0;
  np->lab4_read_lock_held = 0;
  np->lab4_write_lock_held = 0;
  np->lab4_ticket_waiting = 0;
  np->lab4_ticket_held = 0;
  np->lab4_ticket_number = 0;
  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
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

  // Release or cancel any Lab 4 synchronization state before this process
  // becomes a zombie. This prevents killed owners from blocking the system.
  lab4sync_cleanup(curproc);

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
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->sched_exit_tick = ticks;
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and optionally return its scheduler metrics.
// Return -1 if this process has no children.
static int
wait_internal(struct sched_stats *stats)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        pid = p->pid;
        if(stats){
          stats->pid = p->pid;
          stats->queue = p->sched_queue;
          stats->created_tick = p->sched_created_tick;
          stats->first_run_tick = p->sched_first_run_tick;
          stats->exit_tick = p->sched_exit_tick;
          stats->runtime_ticks = p->sched_runtime_ticks;
          stats->runnable_ticks = p->sched_runnable_ticks;
          stats->dispatches = p->sched_dispatches;
          stats->preemptions = p->sched_preemptions;
        }
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }
    sleep(curproc, &ptable.lock);
  }
}

int
wait(void)
{
  return wait_internal(0);
}

int
waitstats(struct sched_stats *stats)
{
  if(stats == 0)
    return -1;
  return wait_internal(stats);
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
  int tried;

  c->proc = 0;
  if(!sched_valid_queue(c->sched_queue)){
    c->sched_queue = SCHED_Q_RR;
    c->sched_queue_ticks = 0;
    c->sched_rr_next = 0;
  }

  for(;;){
    // Enable interrupts on this processor.
    sti();

    acquire(&ptable.lock);

    p = 0;
    for(tried = 0; tried < SCHED_NQUEUE; tried++){
      if(c->sched_queue_ticks >= sched_queue_quota(c->sched_queue) ||
         !sched_queue_has_runnable(c->sched_queue)){
        sched_advance_queue(c);
        continue;
      }

      p = sched_pick_from_queue(c, c->sched_queue);
      if(p != 0)
        break;

      sched_advance_queue(c);
    }

    if(p != 0){
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      p->sched_wait_ticks = 0;
      p->sched_consecutive_ticks = 0;
      if(p->sched_first_run_tick == ~0U)
        p->sched_first_run_tick = ticks;
      p->sched_dispatches++;

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
      sched_mark_runnable_after_sleep(p);
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

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        sched_mark_runnable_after_sleep(p);
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int
procinfo(int pid)
{
  struct proc *p;
  struct proc *q;
  int found = 0;
  int parent_pid = -1;
  int children[NPROC];
  int siblings[NPROC];
  int nchildren = 0;
  int nsiblings = 0;
  int depth = 0;
  char *state = "???";
  int i;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != UNUSED && p->pid == pid){
      found = 1;
      break;
    }
  }

  if(!found){
    release(&ptable.lock);
    return -1;
  }

  parent_pid = p->parent ? p->parent->pid : -1;

  for(q = ptable.proc; q < &ptable.proc[NPROC]; q++){
    if(q->state != UNUSED && q->parent == p && nchildren < NPROC)
      children[nchildren++] = q->pid;
  }

  if(p->parent){
    for(q = ptable.proc; q < &ptable.proc[NPROC]; q++){
      if(q->state != UNUSED && q->parent == p->parent && q != p && nsiblings < NPROC)
        siblings[nsiblings++] = q->pid;
    }
  }

  for(q = p; q->parent; q = q->parent)
    depth++;

  if(p->state >= 0 && p->state < NELEM(pstate_names) && pstate_names[p->state])
    state = pstate_names[p->state];

  release(&ptable.lock);

  cprintf("Process_id: %d\n", pid);
  cprintf("Parent_id: %d\n", parent_pid);

  cprintf("Children's_id: ");
  if(nchildren == 0){
    cprintf("none");
  } else {
    for(i = 0; i < nchildren; i++){
      if(i > 0)
        cprintf(", ");
      cprintf("%d", children[i]);
    }
  }
  cprintf("\n");

  cprintf("Siblings_id: ");
  if(nsiblings == 0){
    cprintf("none");
  } else {
    for(i = 0; i < nsiblings; i++){
      if(i > 0)
        cprintf(", ");
      cprintf("%d", siblings[i]);
    }
  }
  cprintf("\n");

  cprintf("Depth: %d\n", depth);
  cprintf("State: %s\n", state);

  return 0;
}

int
scheduler_tick(void)
{
  struct proc *p;
  struct cpu *c;
  int should_yield = 0;

  p = myproc();
  if(p == 0)
    return 0;

  c = mycpu();
  acquire(&ptable.lock);
  if(p->state == RUNNING){
    if(c->sched_queue != p->sched_queue){
      c->sched_queue = p->sched_queue;
      c->sched_queue_ticks = 0;
    }
    p->sched_consecutive_ticks++;
    p->sched_runtime_ticks++;
    c->sched_queue_ticks++;

    if(p->sched_queue == SCHED_Q_RR &&
       p->sched_consecutive_ticks >= SCHED_RR_QUANTUM_TICKS)
      should_yield = 1;

    if(c->sched_queue_ticks >= sched_queue_quota(c->sched_queue))
      should_yield = 1;
  }
  if(should_yield)
    p->sched_preemptions++;
  release(&ptable.lock);
  return should_yield;
}

void
scheduler_waiting_tick(void)
{
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE)
      continue;

    p->sched_wait_ticks++;
    if(p->sched_queue == SCHED_Q_FCFS && p->sched_wait_ticks >= SCHED_AGING_TICKS){
      p->sched_queue = SCHED_Q_SJF;
      p->sched_wait_ticks = 0;
      p->sched_arrival_tick = ticks;
      // Keep the timer path quiet and deterministic; the updated queue is
      // visible through print_scheduling_info().
    } else if(p->sched_queue == SCHED_Q_SJF && p->sched_wait_ticks >= SCHED_AGING_TICKS){
      p->sched_queue = SCHED_Q_RR;
      p->sched_wait_ticks = 0;
      p->sched_arrival_tick = ticks;
      // Keep the timer path quiet and deterministic; the updated queue is
      // visible through print_scheduling_info().
    }
  }
  release(&ptable.lock);
}

int
scheduler_set_params(int pid, int burst, int confidence)
{
  struct proc *p;

  if(burst <= 0 || confidence < 0 || confidence > 100)
    return -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != UNUSED && p->pid == pid){
      p->sched_burst_time = burst;
      p->sched_confidence = confidence;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int
scheduler_change_queue(int pid, int q)
{
  struct proc *p;

  if(!sched_valid_queue(q))
    return -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != UNUSED && p->pid == pid){
      if(p->sched_queue == q){
        release(&ptable.lock);
        return 0;
      }
      p->sched_queue = q;
      p->sched_arrival_tick = ticks;
      p->sched_consecutive_ticks = 0;
      if(p == myproc() && p->state == RUNNING){
        struct cpu *c = mycpu();
        c->sched_queue = q;
        c->sched_queue_ticks = 0;
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int
scheduler_print_info(void)
{
  struct proc *p;
  char *state;

  acquire(&ptable.lock);
  cprintf("name            pid state     queue wait confidence burst consecutive arrival\n");
  cprintf("--------------- --- --------- ----- ---- ---------- ----- ----------- -------\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(pstate_names) && pstate_names[p->state])
      state = pstate_names[p->state];
    else
      state = "???";
    cprintf("%s", p->name);
    // fixed-width strings are inconvenient in xv6 cprintf, so columns are space separated.
    cprintf(" %d %s %d %d %d %d %d %d\n",
            p->pid, state, p->sched_queue, p->sched_wait_ticks,
            p->sched_confidence, p->sched_burst_time,
            p->sched_consecutive_ticks, p->sched_arrival_tick);
  }
  release(&ptable.lock);
  return 0;
}

void
scheduler_on_exec(char *name)
{
  struct proc *p = myproc();

  acquire(&ptable.lock);
  if(p == initproc || strncmp(name, "init", 5) == 0 || strncmp(name, "sh", 3) == 0){
    p->sched_queue = SCHED_Q_RR;
    p->sched_shell_path = 1;
  } else {
    p->sched_queue = SCHED_Q_FCFS;
    p->sched_shell_path = 0;
  }
  p->sched_wait_ticks = 0;
  p->sched_consecutive_ticks = 0;
  p->sched_arrival_tick = ticks;
  release(&ptable.lock);
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(pstate_names) && pstate_names[p->state])
      state = pstate_names[p->state];
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

// int
// procinfo(int pid)
// {
//   struct proc *p;
//   struct proc *iter;
//   int found = 0;
//   int depth;
//   int first;  // helper for printing commas

//   acquire(&ptable.lock);

//   // 1. Find the target process
//   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//     if(p->pid == pid){
//       found = 1;
//       break;
//     }
//   }

//   if(!found){
//     release(&ptable.lock);
//     return -1;
//   }

//   // 2. Print basic info
//   cprintf("Process_id: %d\n", p->pid);
//   if(p->parent)
//     cprintf("Parent_id: %d\n", p->parent->pid);
//   else
//     cprintf("Parent_id: none\n");

//   // 3. Children
//   cprintf("Children's_id: ");
//   first = 1;
//   for(iter = ptable.proc; iter < &ptable.proc[NPROC]; iter++){
//     if(iter->parent == p && iter->state != UNUSED){
//       if(!first)
//         cprintf(", ");
//       cprintf("%d", iter->pid);
//       first = 0;
//     }
//   }
//   if(first)  // no children
//     cprintf("none");
//   cprintf("\n");

//   // 4. Siblings (same parent, different pid)
//   cprintf("Siblings_id: ");
//   first = 1;
//   if(p->parent){
//     for(iter = ptable.proc; iter < &ptable.proc[NPROC]; iter++){
//       if(iter->parent == p->parent && iter->pid != pid && iter->state != UNUSED){
//         if(!first)
//           cprintf(", ");
//         cprintf("%d", iter->pid);
//         first = 0;
//       }
//     }
//   }
//   if(first)
//     cprintf("none");
//   cprintf("\n");

//   // 5. Depth in process tree (distance to init, pid 1)
//   depth = 0;
//   if(p->pid > 1){
//     struct proc *anc = p->parent;
//     while(anc && anc->pid > 1){
//       depth++;
//       anc = anc->parent;
//     }
//   }
//   cprintf("Depth: %d\n", depth);

//   // 6. State
//   static char *states[] = {
//     [UNUSED]   "UNUSED",
//     [EMBRYO]   "EMBRYO",
//     [SLEEPING] "SLEEPING",
//     [RUNNABLE] "RUNNABLE",
//     [RUNNING]  "RUNNING",
//     [ZOMBIE]   "ZOMBIE"
//   };
//   if(p->state >= 0 && p->state < NELEM(states))
//     cprintf("State: %s\n", states[p->state]);
//   else
//     cprintf("State: unknown\n");

//   release(&ptable.lock);
//   return 0;
// }
