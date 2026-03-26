# RESULTS

## Context Switching Approach
I implemented user-level cooperative threads in user space.
Each thread has its own stack and saved context.
The saved context stores eip, esp, ebp, ebx, esi, and edi.

A small assembly routine saves the current thread context
and restores the next runnable thread context.

## Scheduling
The scheduler is cooperative round-robin.
Threads switch only when they explicitly call thread_yield(),
block in thread_join(), or wait in mutex_lock().

## Join
thread_join(tid) blocks the calling thread until the target thread finishes.
When a thread finishes, it becomes T_ZOMBIE and any blocked joiners waiting
for that thread are woken up.

## Mutex
The mutex is a simple user-level lock.
If the lock is already held, the caller repeatedly yields until it becomes free.

## Limitations
- Maximum threads: 8
- Stack size per thread: 4096 bytes
- Cooperative only, no timer-based preemption
- Mutex works for cooperative user threads