#define GET_PID(x)    (((x) >> 1) & 0x7FF)
#define SWAP_PERM(x)  ((x) & 1)

#define PACK(vpn, pid, sw) ((vpn) | ((pid) << 1) | sw)

#define RAMMAP_PAGES ((PHYSTOP - EXTMEM) / PGSIZE)
#define RDX_2_PA(rdx)  (EXTMEM + (rdx << 12))
#define PA_2_RDX(pa)   ((pa - EXTMEM) >> 12)

#define SWAPSIZE 469762048
#define SWAP_PAGES SWAPSIZE / PGSIZE
#define MAX_REFCOUNT 255
#define GET_SWAPSLOT(pte) (PTE_ADDR(pte) >> 12)
