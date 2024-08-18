// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#define REF_IDX(pa) ((pa-KERNBASE)/PGSIZE)

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
extern uint refer_count[];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int refer_count[REF_IDX(PHYSTOP)];
} kref;

void acquire_lock()
{
  acquire(&kref.lock);
}

void release_lock()
{
  release(&kref.lock);
}

void init_ref()
{
  acquire(&kref.lock);
  for(int i=0;i<REF_IDX(PHYSTOP);i++)
    kref.refer_count[i]=0;
  release(&kref.lock);
}

int add_ref(void* pa)
{
  return ++kref.refer_count[REF_IDX((uint64)pa)];
}

int sub_ref(void* pa)
{
  return --kref.refer_count[REF_IDX((uint64)pa)];
}
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kref.lock,"kref");
  init_ref();
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if(sub_ref(pa)>0)
    return;
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    acquire(&kref.lock);
    kref.refer_count[REF_IDX((uint64)r)]=1;
    release(&kref.lock);
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}
