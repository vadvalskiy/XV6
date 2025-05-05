#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define THREAD_COUNT 4
#define WORK_ITERATIONS 100
#define YIELD_FREQUENCY 10

int shared_value = 0;
lock_t shared_lock;
int per_thread_work[THREAD_COUNT];

void worker_thread(void *arg1, void *arg2) {
  int thread_id = *(int*)arg1;
  int iterations = *(int*)arg2;
  int local_counter = 0;
  
  printf(1, "Worker %d: Starting execution (%d iterations)\n", thread_id, iterations);
  
  for (int i = 0; i < iterations; i++) {
    lock_acquire(&shared_lock);
    shared_value++;
    local_counter++;
    lock_release(&shared_lock);
    
    if (i % YIELD_FREQUENCY == 0) {
      sleep(1);
    }
  }
  
  lock_acquire(&shared_lock);
  per_thread_work[thread_id] = local_counter;
  lock_release(&shared_lock);
  
  printf(1, "Worker %d: Completed %d increments\n", thread_id, local_counter);
  exit();
}

void recursive_spawner(void *arg1, void *arg2) {
  int depth = *(int*)arg1;
  int max_depth = *(int*)arg2;
  int child_depth = depth + 1;
  int child_iters = 20;
  
  printf(1, "Spawner at depth %d (max: %d)\n", depth, max_depth);
  
  if (depth >= max_depth) {
    printf(1, "Spawner reached max depth %d, exiting\n", depth);
    exit();
  }
  
  thread_create(recursive_spawner, (void*)&child_depth, (void*)max_depth);
  thread_create(worker_thread, (void*)&depth, (void*)&child_iters);
  
  thread_join();
  thread_join();
  
  printf(1, "Spawner at depth %d: Children completed\n", depth);
  exit();
}

int main(void) {
  int thread_ids[THREAD_COUNT];
  int work_per_thread = WORK_ITERATIONS;
  
  printf(1, "=== Thread Implementation Test Program ===\n");
  printf(1, "Threads: %d, Work iterations: %d\n\n", THREAD_COUNT, work_per_thread);
  
  if (lock_init(&shared_lock) != 0) {
    printf(2, "ERROR: Failed to initialize lock\n");
    exit();
  }
  
  printf(1, "--- Basic Thread Test ---\n");
  
  for (int i = 0; i < THREAD_COUNT; i++) {
    thread_ids[i] = i;
    int pid = thread_create(worker_thread, (void*)&thread_ids[i], (void*)&work_per_thread);
    printf(1, "Created thread %d with PID %d\n", i, pid);
  }
  
  for (int i = 0; i < THREAD_COUNT; i++) {
    thread_join();
  }
  
  printf(1, "\nThread work verification:\n");
  int total_work = 0;
  for (int i = 0; i < THREAD_COUNT; i++) {
    printf(1, "Thread %d performed %d operations\n", i, per_thread_work[i]);
    total_work += per_thread_work[i];
  }
  
  printf(1, "\nShared value: %d, Sum of thread work: %d\n", 
         shared_value, total_work);
         
  if (shared_value == total_work) {
    printf(1, "RESULT: Basic thread test PASSED\n");
  } else {
    printf(1, "RESULT: Basic thread test FAILED\n");
  }
  
  shared_value = 0;
  
  printf(1, "\n--- Recursive Thread Creation Test ---\n");
  
  int start_depth = 0;
  int max_depth = 3;
  
  thread_create(recursive_spawner, (void*)&start_depth, (void*)&max_depth);
  thread_join();
  
  printf(1, "Recursive thread test completed\n");
  
  printf(1, "\n=== All Tests Completed ===\n");
  exit();
}
