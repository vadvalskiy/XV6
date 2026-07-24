#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
// Lab4 syscall-counting configuration.
// 0: shared global counter without lock, 1: shared global counter with spinlock,
// 2: per-CPU counters.  The final implementation uses the scalable per-CPU mode.
#define SYSCALL_COUNT_GLOBAL_UNLOCKED 0
#define SYSCALL_COUNT_GLOBAL_LOCKED   1
#define SYSCALL_COUNT_PERCPU          2
#ifndef SYSCALL_COUNT_MODE
#define SYSCALL_COUNT_MODE            SYSCALL_COUNT_PERCPU
#endif
#define NSYSCALL      64  // size of syscall counter arrays

