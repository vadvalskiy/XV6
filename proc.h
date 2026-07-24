// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null

  // Lab 3: state of this CPU's weighted round-robin scheduler.
  int sched_queue;             // queue currently receiving this CPU's slice
  int sched_queue_ticks;       // ticks spent in the current queue slice
  int sched_rr_next;           // next process-table index for queue-0 RR
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Lab 3 scheduler constants.  Queue 0 has the highest priority,
// but queues are selected by per-CPU weighted round robin rather than
// by fixed priority.
#define SCHED_Q_RR     0
#define SCHED_Q_SJF    1
#define SCHED_Q_FCFS   2
#define SCHED_NQUEUE   3
#define SCHED_RR_QUANTUM_TICKS 5     // 50 ms when one tick is 10 ms
#define SCHED_WRR_UNIT_TICKS   10    // 100 ms
#define SCHED_AGING_TICKS      800

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)

  // Lab 3 scheduling metadata.
  int sched_queue;             // 0: RR, 1: approximate SJF, 2: FCFS
  int sched_wait_ticks;        // RUNNABLE ticks since last dispatch/promotion
  int sched_burst_time;        // estimated burst for SJF; default 2
  int sched_confidence;        // confidence in burst estimate; default 50
  int sched_consecutive_ticks; // ticks in the current uninterrupted run
  uint sched_arrival_tick;     // time of entering the current queue
  int sched_shell_path;        // init/sh lineage flag used to keep shell live
  uint sched_created_tick;     // start of the current measurement epoch
  uint sched_first_run_tick;   // first dispatch, or ~0U when never dispatched
  uint sched_exit_tick;        // tick at process exit
  uint sched_runtime_ticks;    // timer ticks observed while RUNNING
  uint sched_runnable_ticks;   // timer ticks observed while RUNNABLE
  uint sched_dispatches;       // scheduler dispatch count
  uint sched_preemptions;      // timer-driven yield count
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
