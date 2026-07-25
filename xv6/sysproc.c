
#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "schedstat.h"
#include "spinlock.h"
#include "fcntl.h"
#include "file.h"

extern uint ticks;
extern struct spinlock tickslock;

static struct spinlock randlock;
static int randinit;
static uint random_state;


#define PC_BUFFER_SIZE 16

struct pc_buffer {
  int buffer[PC_BUFFER_SIZE];
  int head;
  int tail;
  int count;
  int not_full;
  int not_empty;
  struct spinlock lock;
};

struct rwlock {
  int readers;
  int active_writer;
  int waiting_writers;
  int writer_pid;
  int reader_chan;
  int writer_chan;
  struct spinlock lock;
};

struct ticket_cancel {
  uint ticket;
  int valid;
};

struct ticketLock {
  uint next_ticket;
  uint turn;
  int owner_pid;
  uint owner_ticket;
  int turn_chan;
  int space_chan;
  struct ticket_cancel canceled[NPROC];
  struct spinlock lock;
};

static struct pc_buffer pcbuf;
static struct rwlock lab4rw;
static struct ticketLock lab4ticket;

static void
pc_buffer_init(struct pc_buffer *b)
{
  b->head = 0;
  b->tail = 0;
  b->count = 0;
  b->not_full = 0;
  b->not_empty = 0;
  initlock(&b->lock, "pcbuf");
}

static void
rwlock_init(struct rwlock *rw)
{
  rw->readers = 0;
  rw->active_writer = 0;
  rw->waiting_writers = 0;
  rw->writer_pid = 0;
  rw->reader_chan = 0;
  rw->writer_chan = 0;
  initlock(&rw->lock, "rwlock");
}

static void
ticketLock_init(struct ticketLock *tl)
{
  int i;
  tl->next_ticket = 0;
  tl->turn = 0;
  tl->owner_pid = 0;
  tl->owner_ticket = 0;
  tl->turn_chan = 0;
  tl->space_chan = 0;
  for(i = 0; i < NPROC; i++)
    tl->canceled[i].valid = 0;
  initlock(&tl->lock, "ticket");
}

void
lab4syncinit(void)
{
  pc_buffer_init(&pcbuf);
  rwlock_init(&lab4rw);
  ticketLock_init(&lab4ticket);
}

static int
pc_try_produce_value(int value)
{
  acquire(&pcbuf.lock);
  if(pcbuf.count == PC_BUFFER_SIZE){
    release(&pcbuf.lock);
    return -1;
  }
  pcbuf.buffer[pcbuf.tail] = value;
  pcbuf.tail = (pcbuf.tail + 1) % PC_BUFFER_SIZE;
  pcbuf.count++;
  wakeup(&pcbuf.not_empty);
  release(&pcbuf.lock);
  return 0;
}

static int
pc_try_consume_value(int *value)
{
  acquire(&pcbuf.lock);
  if(pcbuf.count == 0){
    release(&pcbuf.lock);
    return -1;
  }
  *value = pcbuf.buffer[pcbuf.head];
  pcbuf.head = (pcbuf.head + 1) % PC_BUFFER_SIZE;
  pcbuf.count--;
  wakeup(&pcbuf.not_full);
  release(&pcbuf.lock);
  return 0;
}

static int
pc_produce_value(int value)
{
  acquire(&pcbuf.lock);
  while(pcbuf.count == PC_BUFFER_SIZE){
    if(myproc()->killed){
      release(&pcbuf.lock);
      return -1;
    }
    sleep(&pcbuf.not_full, &pcbuf.lock);
  }
  pcbuf.buffer[pcbuf.tail] = value;
  pcbuf.tail = (pcbuf.tail + 1) % PC_BUFFER_SIZE;
  pcbuf.count++;
  wakeup(&pcbuf.not_empty);
  release(&pcbuf.lock);
  return 0;
}

static int
pc_consume_value(int *value)
{
  acquire(&pcbuf.lock);
  while(pcbuf.count == 0){
    if(myproc()->killed){
      release(&pcbuf.lock);
      return -1;
    }
    sleep(&pcbuf.not_empty, &pcbuf.lock);
  }
  *value = pcbuf.buffer[pcbuf.head];
  pcbuf.head = (pcbuf.head + 1) % PC_BUFFER_SIZE;
  pcbuf.count--;
  wakeup(&pcbuf.not_full);
  release(&pcbuf.lock);
  return 0;
}

static int
rwlock_acquire_read(struct rwlock *rw)
{
  struct proc *p = myproc();
  if(p->lab4_read_lock_held || p->lab4_write_lock_held)
    return -1;
  acquire(&rw->lock);
  while(rw->active_writer || rw->waiting_writers > 0){
    if(p->killed){
      release(&rw->lock);
      return -1;
    }
    sleep(&rw->reader_chan, &rw->lock);
  }
  rw->readers++;
  p->lab4_read_lock_held = 1;
  release(&rw->lock);
  return 0;
}

static int
rwlock_release_read(struct rwlock *rw)
{
  struct proc *p = myproc();
  acquire(&rw->lock);
  if(!p->lab4_read_lock_held || rw->readers <= 0){
    release(&rw->lock);
    return -1;
  }
  p->lab4_read_lock_held = 0;
  rw->readers--;
  if(rw->readers == 0)
    wakeup(&rw->writer_chan);
  release(&rw->lock);
  return 0;
}

static int
rwlock_acquire_write(struct rwlock *rw)
{
  struct proc *p = myproc();
  if(p->lab4_read_lock_held || p->lab4_write_lock_held)
    return -1;
  acquire(&rw->lock);
  rw->waiting_writers++;
  while(rw->active_writer || rw->readers > 0){
    if(p->killed){
      rw->waiting_writers--;
      if(rw->waiting_writers == 0)
        wakeup(&rw->reader_chan);
      release(&rw->lock);
      return -1;
    }
    sleep(&rw->writer_chan, &rw->lock);
  }
  rw->waiting_writers--;
  rw->active_writer = 1;
  rw->writer_pid = p->pid;
  p->lab4_write_lock_held = 1;
  release(&rw->lock);
  return 0;
}

static int
rwlock_release_write(struct rwlock *rw)
{
  struct proc *p = myproc();
  acquire(&rw->lock);
  if(!p->lab4_write_lock_held || !rw->active_writer || rw->writer_pid != p->pid){
    release(&rw->lock);
    return -1;
  }
  p->lab4_write_lock_held = 0;
  rw->active_writer = 0;
  rw->writer_pid = 0;
  if(rw->waiting_writers > 0)
    wakeup(&rw->writer_chan);
  else
    wakeup(&rw->reader_chan);
  release(&rw->lock);
  return 0;
}

static void
ticket_skip_canceled_locked(struct ticketLock *tl)
{
  int slot;
  for(;;){
    slot = tl->turn % NPROC;
    if(!tl->canceled[slot].valid || tl->canceled[slot].ticket != tl->turn)
      break;
    tl->canceled[slot].valid = 0;
    tl->turn++;
  }
}

static void
ticket_cancel_locked(struct ticketLock *tl, uint ticket)
{
  int slot = ticket % NPROC;
  tl->canceled[slot].ticket = ticket;
  tl->canceled[slot].valid = 1;
  ticket_skip_canceled_locked(tl);
  wakeup(&tl->turn_chan);
  wakeup(&tl->space_chan);
}

static int
ticketLock_acquire(struct ticketLock *tl)
{
  struct proc *p = myproc();
  uint mine;

  if(p->lab4_ticket_waiting || p->lab4_ticket_held)
    return -1;
  acquire(&tl->lock);
  while(tl->next_ticket - tl->turn >= NPROC){
    if(p->killed){
      release(&tl->lock);
      return -1;
    }
    sleep(&tl->space_chan, &tl->lock);
  }
  mine = tl->next_ticket++;
  p->lab4_ticket_waiting = 1;
  p->lab4_ticket_number = mine;

  while(tl->turn != mine){
    if(p->killed){
      p->lab4_ticket_waiting = 0;
      ticket_cancel_locked(tl, mine);
      release(&tl->lock);
      return -1;
    }
    sleep(&tl->turn_chan, &tl->lock);
  }
  if(tl->owner_pid != 0){
    p->lab4_ticket_waiting = 0;
    ticket_cancel_locked(tl, mine);
    release(&tl->lock);
    return -1;
  }
  p->lab4_ticket_waiting = 0;
  p->lab4_ticket_held = 1;
  tl->owner_pid = p->pid;
  tl->owner_ticket = mine;
  release(&tl->lock);
  return (int)mine;
}

static int
ticketLock_release(struct ticketLock *tl)
{
  struct proc *p = myproc();
  acquire(&tl->lock);
  if(!p->lab4_ticket_held || tl->owner_pid != p->pid ||
     tl->owner_ticket != p->lab4_ticket_number){
    release(&tl->lock);
    return -1;
  }
  p->lab4_ticket_held = 0;
  tl->owner_pid = 0;
  tl->owner_ticket = 0;
  tl->turn++;
  ticket_skip_canceled_locked(tl);
  wakeup(&tl->turn_chan);
  wakeup(&tl->space_chan);
  release(&tl->lock);
  return 0;
}

static int
ticketLock_turn(struct ticketLock *tl)
{
  int turn;
  acquire(&tl->lock);
  turn = (int)tl->turn;
  release(&tl->lock);
  return turn;
}

void
lab4sync_cleanup(struct proc *p)
{
  acquire(&lab4rw.lock);
  if(p->lab4_read_lock_held){
    p->lab4_read_lock_held = 0;
    if(lab4rw.readers > 0)
      lab4rw.readers--;
    if(lab4rw.readers == 0)
      wakeup(&lab4rw.writer_chan);
  }
  if(p->lab4_write_lock_held && lab4rw.active_writer && lab4rw.writer_pid == p->pid){
    p->lab4_write_lock_held = 0;
    lab4rw.active_writer = 0;
    lab4rw.writer_pid = 0;
    if(lab4rw.waiting_writers > 0)
      wakeup(&lab4rw.writer_chan);
    else
      wakeup(&lab4rw.reader_chan);
  }
  release(&lab4rw.lock);

  acquire(&lab4ticket.lock);
  if(p->lab4_ticket_waiting){
    p->lab4_ticket_waiting = 0;
    ticket_cancel_locked(&lab4ticket, p->lab4_ticket_number);
  }
  if(p->lab4_ticket_held && lab4ticket.owner_pid == p->pid){
    p->lab4_ticket_held = 0;
    lab4ticket.owner_pid = 0;
    lab4ticket.owner_ticket = 0;
    lab4ticket.turn++;
    ticket_skip_canceled_locked(&lab4ticket);
    wakeup(&lab4ticket.turn_chan);
    wakeup(&lab4ticket.space_chan);
  }
  release(&lab4ticket.lock);
}

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
sys_waitstats(void)
{
  struct sched_stats *stats;

  if(argptr(0, (char**)&stats, sizeof(*stats)) < 0)
    return -1;
  return waitstats(stats);
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
  uint values[14];

  randominit_once();

  if(argint(0, &n) < 0)
    return -1;
  if(n <= 0 || n >= 15)
    return -1;
  if(argptr(1, (char**)&ubuf, n * sizeof(int)) < 0)
    return -1;

  // Keep the PRNG state critical section short. User-memory access may fault,
  // so copy only after releasing the spinlock.
  acquire(&randlock);
  for(i = 0; i < n; i++){
    random_state = (1103515245U * random_state + 12345U) & 0x7fffffffU;
    values[i] = random_state;
  }
  release(&randlock);

  if(copyout(myproc()->pgdir, (uint)ubuf, values, n * sizeof(values[0])) < 0)
    return -1;
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




int
sys_set_scheduling_info(void)
{
  int pid, burst, confidence;

  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &burst) < 0)
    return -1;
  if(argint(2, &confidence) < 0)
    return -1;
  return scheduler_set_params(pid, burst, confidence);
}

int
sys_change_queue(void)
{
  int pid, q;

  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &q) < 0)
    return -1;
  return scheduler_change_queue(pid, q);
}

int
sys_print_scheduling_info(void)
{
  return scheduler_print_info();
}

int
sys_getcount(void)
{
  int syscall_number;

  if(argint(0, &syscall_number) < 0)
    return -1;
  return syscall_count_get(syscall_number);
}

int
sys_getcpucount(void)
{
  int cpu_id;
  int syscall_number;

  if(argint(0, &cpu_id) < 0)
    return -1;
  if(argint(1, &syscall_number) < 0)
    return -1;
  return syscall_count_getcpu(cpu_id, syscall_number);
}

int
sys_getcountmode(void)
{
  return syscall_count_mode();
}

int
sys_produce(void)
{
  int value;
  if(argint(0, &value) < 0)
    return -1;
  return pc_produce_value(value);
}

int
sys_try_produce(void)
{
  int value;
  if(argint(0, &value) < 0)
    return -1;
  return pc_try_produce_value(value);
}

int
sys_try_consume(void)
{
  int *out;
  int value;
  if(argptr(0, (char**)&out, sizeof(*out)) < 0)
    return -1;
  if(pc_try_consume_value(&value) < 0)
    return -1;
  *out = value;
  return 0;
}

int
sys_consume_value(void)
{
  int *out;
  int value;
  if(argptr(0, (char**)&out, sizeof(*out)) < 0)
    return -1;
  if(pc_consume_value(&value) < 0)
    return -1;
  *out = value;
  return 0;
}

int
sys_consume(void)
{
  int value;
  if(pc_consume_value(&value) < 0)
    return -1;
  return value;
}

int
sys_rw_acquire_read(void)
{
  return rwlock_acquire_read(&lab4rw);
}

int
sys_rw_release_read(void)
{
  return rwlock_release_read(&lab4rw);
}

int
sys_rw_acquire_write(void)
{
  return rwlock_acquire_write(&lab4rw);
}

int
sys_rw_release_write(void)
{
  return rwlock_release_write(&lab4rw);
}

int
sys_ticket_acquire(void)
{
  return ticketLock_acquire(&lab4ticket);
}

int
sys_ticket_release(void)
{
  return ticketLock_release(&lab4ticket);
}

int
sys_ticket_turn(void)
{
  return ticketLock_turn(&lab4ticket);
}

