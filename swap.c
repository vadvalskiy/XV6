#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

struct spinlock ramlock;
struct spinlock swaplock;

uint rammap[RAMMAP_PAGES];
uint swapmap[NELEM_SWAPMAP];

void
raminit()
{
  initlock(&ramlock, "rammap");
}

void
swapinit()
{
  initlock(&swaplock, "swapmap");
}

// Return offset + 1
// Just to avoid negative integers
uint
find_free_swapslot(void)
{
  acquire(&swaplock);
  for(int i = 0; i < NELEM_SWAPMAP; i++) {
    if(swapmap[i] != 0xFFFFFFFF) {
      uint idx = __builtin_ctz(~swapmap[i]);
      swapmap[i] |= (1 << idx);
      release(&swaplock);
      return i * 32 + idx + 1;
    }
  }
  release(&swaplock);
  return 0;
}

void
read_from_swap(char *buf, uint off)
{
  
}

uint
write_to_swap(char *buf)
{
  uint ss_off;
  acquire(&swaplock);
  if(!(ss_off = find_free_swapslot()))
    return 0;
}
