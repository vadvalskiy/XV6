#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc() && myproc()->killed)
      exit();
    if(myproc())
      myproc()->tf = tf;
    syscall();
    if(myproc() && myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;

  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;

  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;

  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, attempt to handle page faults by allocating a page
    // and mapping it at the faulting address so simple programs that
    // rely on sbrk/malloc will continue to work. If we can't handle
    // it here, fall through and kill the process as before.
#ifdef ALLOCATOR_LOCALITY
    if(tf->trapno == T_PGFLT){
      uint fault_addr = rcr2();
      struct proc *p = myproc();
      // Only handle faults in user address space and within process size
      if(p && fault_addr < KERNBASE && fault_addr < p->sz){
        uint va = PGROUNDDOWN(fault_addr);
        // Try to allocate three pages starting at the faulting address
        uint end_va = va + 3*PGSIZE;
        // Don't allocate past KERNBASE or process size
        if(end_va > KERNBASE || end_va > p->sz){
          uint limit = p->sz;
          if(limit > KERNBASE)
            limit = KERNBASE;
          end_va = PGROUNDDOWN(limit);
        }
        cprintf("pgfault: pid %d locality alloc addr 0x%x -> pages 0x%x-0x%x\n",
                p->pid, fault_addr, va, end_va - 1);
        if(allocuvm(p->pgdir, va, end_va) != 0){
          cprintf("pgfault: pid %d locality mapped 0x%x-0x%x\n",
                  p->pid, va, end_va - 1);
          return;  // Success - mapped up to 3 pages
        }
        cprintf("pgfault: pid %d locality alloc failed for 0x%x-0x%x\n",
                p->pid, va, end_va - 1);
      }
    }
#else  // ALLOCATOR_LAZY
    if(tf->trapno == T_PGFLT){
      uint fault_addr = rcr2();
      struct proc *p = myproc();
      // Only handle faults in user address space and within process size
      if(p && fault_addr < KERNBASE && fault_addr < p->sz){
        uint va = PGROUNDDOWN(fault_addr);
        cprintf("pgfault: pid %d lazy alloc addr 0x%x -> page 0x%x\n",
                p->pid, fault_addr, va);
        if(allocuvm(p->pgdir, va, va + PGSIZE) != 0){
          cprintf("pgfault: pid %d lazy mapped 0x%x\n", p->pid, va);
          return;  // Success - mapped one page
        }
        cprintf("pgfault: pid %d lazy alloc failed for 0x%x\n", p->pid, va);
      }
    }
#endif
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER) {

    myproc()->ticks_running++;

#ifdef SCHEDULER_PRIORITYRR
    // Quantum-based preemption for PRIORITYRR
    myproc()->rrticks++;
    if(myproc()->rrticks >= myproc()->quantum){
      // reset slice counter before yielding so next run starts fresh
      myproc()->rrticks = 0;
      yield();
    }
#else
    yield();
#endif
  }
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
