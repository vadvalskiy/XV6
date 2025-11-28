#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "buf.h"

struct spinlock ramlock;
struct spinlock swaplock;

uint rammap[RAMMAP_PAGES];
uint8_t swapmap[SWAP_PAGES];

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
static uint
find_free_swapslot(void)
{
  acquire(&swaplock);
  for(int i = 0; i < SWAP_PAGES; i++) {
    if(swapmap[i] == 0) {
      swapmap[i]++;
      release(&swaplock);
      return i + 1;
    }
  }
  release(&swaplock);
  return 0;
}

void
read_from_swap(uint page_index, char *dst)
{
	page_index--;
  uint sector_start = page_index * 8;
  for(int i = 0; i < 8; i++){
      struct buf *b = bread(2, sector_start + i);
      memmove(dst + (i * 512), b->data, 512);
      brelse(b);
  }
  uint idx=page_index/32;
  uint off=page_index-idx*32;
  acquire(&swaplock);
	swapmap[idx]--;
	release(&swaplock);
}

void
write_to_swap(char *page)
{
  uint sector_start = (find_free_slot()-1)*8;

  for(int i = 0; i < 8; i++){
    struct buf *b = bread(2,sector_start + i);
    memmove(b->data, page + (i * 512), 512);
    bwrite(b);
    brelse(b);
  }
}
