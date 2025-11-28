// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);

  // Mark page as free in rammap.
  acquire(&ramlock);
  rammap[PA_2_RDX(V2P((uint)v))] = 0;
  release(&ramlock);
}

// Returns the PA of the evicted page in RAM
static uint
evict_page(void)
{
  uint victim_entry = 0;
  uint victim_idx = -1;

  // Choose a victim to evict
  acquire(&ramlock);
  for(int i = RAMMAP_PAGES - 1; i >= 0; i--) {
    if(SWAP_BIT(rammap[i])) {
      victim_entry = rammap[i];
      victim_idx = i;
      break;
    }
  }
  release(&ramlock);

  // No swappable pages found
  if(victim_idx < 0)
    return 0;

  // Find out the owner pid and the virtual address
  // which this page is mapped to.
  uint pid = GET_PID(victim_entry);
  uint va = PTE_ADDR(victim_entry);
  uint pa = RDX_2_PA(victim_idx);

  // Fetch the struct proc of owner process
  struct proc *vproc = 0;
  acquire(&ptable.lock);
  for(i = 0; i < NPROC; i++) {
    if(proc[i].pid == pid) {
      vproc = &proc[i];
      break;
    }
  }
  release(&ptable.lock);

  // Write contents of the page to swapspace
  // get the offset.
  acquire(&swaplock);
  uint off = write_to_swap(P2V(pa));
  release(&swaplock);

  // Store the page-aligned swapspace offset into
  // the PTE corresponding to the evicted page in
  // owner process's address space.
  pte_t *pte = walkpgdir(vproc->pgdir, va, 0);
  perms = *pte & ~(0xFFF);
  *pte = ((off << 12) | (perms & ~PTE_P));

  // Finally updated rammap as free.
  acquire(&ramlock);
  rammap[victim_idx] = 0;
  release(&ramlock);

  // Return the physical address of the now free page.
  return pa;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc()
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);

  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;
    if(kmem.use_lock)
      release(&kmem.lock);

    // mark page to be owned by kernel initially
    acquire(&ramlock);
    rammap[PA_2_RDX(V2P(r))] = PACK(0, PID_KERNEL, 0);
    release(&ramlock);

    return (char*)r;
  }

  // No free pages found
  if(kmem.use_lock)
    release(&kmem.lock);

  uint pa = evict_page();
  if(pa == 0)
    return 0;

  acquire(&ramlock);
  rammap[PA_2_RDX(pa)] = PACK(0, PID_KERNEL, 0);
  release(&ramlock);

  return (char*)P2V(pa);
}
