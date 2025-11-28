#define GET_PID(x)    (((x) >> 1) & 0x7FF)
#define SWAP_PERM(x)  ((x) & 1)

#define PACK(ppn, pid, sw) ((ppn) | ((pid) << 1) | sw)

#define RAMMAP_PAGES ((PHYSTOP - EXTMEM) / PGSIZE)
#define RDX_2_PA(uint(rdx))  (EXTMEM + (rdx << 12))
#define PA_2_RDX(uint(pa))   ((pa - EXTMEM) >> 12)

#define SWAPSIZE 469762048
#define SWAP_PAGES SWAPSIZE / PGSIZE
#define NELEM_SWAPMAP ((SWAP_PAGES + 31) / 32)
