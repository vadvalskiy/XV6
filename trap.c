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
// Global variables for priority time quanta
int priority_time[4];
int current_priority = 1;  // Start with priority 1

void init_priority_time() {
  priority_time[1] = 3 * ticks;
  priority_time[2] = 2 * ticks;
  priority_time[3] = 1 * ticks;
}

void trap(struct trapframe *tf) {
  if (tf->trapno == T_SYSCALL) {
    if (myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if (myproc()->killed)
      exit();
    return;
  }

  switch (tf->trapno) {
  case T_IRQ0 + IRQ_TIMER:
    if (cpuid() == 0) {
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

  case T_IRQ0 + IRQ_IDE + 1:
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

  default:
    if (myproc() == 0 || (tf->cs & 3) == 0) {
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }
  // Aging and queue adjustment
  age_stuff();
  // Handle priority-based time-slicing
    if (myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0 + IRQ_TIMER) {
      priority_time[current_priority] -= 1;

      if (priority_time[current_priority] <= 0) {
        // Reset time for the current priority
        if (current_priority == 1) {
          priority_time[1] = 30;
        } else if (current_priority == 2) {
          priority_time[2] = 20;
        } else if (current_priority == 3) {
          priority_time[3] = 10;
        }

        // Move to the next priority
        current_priority++;
        if (current_priority > 3) {
          current_priority = 1;  // Wrap around
        }

        yield();  // Yield the CPU
      }
    }
    
  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();

  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();
}