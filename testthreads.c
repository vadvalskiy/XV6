#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#define SLEEP_TIME 100

lock_t lk;
int execution_order = 0;

void thread_function(void* arg1, void* arg2) {
  int thread_id = *(int*)arg1;
  int use_lock = *(int*)arg2;
  
  if (use_lock) lock_acquire(&lk);
  
  // Record and display execution order when using locks
  if (use_lock) {
    execution_order++;
    printf(1, "Thread %d executing (order: %d)\n", thread_id, execution_order);
    printf(1, "Thread %d sleeping for %d ticks\n", thread_id, SLEEP_TIME);
  } else {
    // Just display a message when not using locks
    printf(1, "Thread %d executing without lock\n", thread_id);
    printf(1, "Thread %d sleeping for %d ticks\n", thread_id, SLEEP_TIME);
  }
  
  // Sleep to make scheduling effects more visible
  sleep(SLEEP_TIME);
  
  if (use_lock) {
    printf(1, "Thread %d finished (order: %d)\n", thread_id, execution_order);
    lock_release(&lk);
  } else {
    printf(1, "Thread %d finished without lock\n", thread_id);
  }
  
  exit();
}

int main(int argc, char *argv[])
{
  // Initialize variables
  lock_init(&lk);
  int thread_ids[3] = {1, 2, 3};
  int use_lock = 1;
  int no_lock = 0;
  
  printf(1, "==== Sequential Execution Test (with locks) ====\n");
  
  // Create three threads with lock
  thread_create(thread_function, (void *)&thread_ids[0], (void *)&use_lock);
  thread_create(thread_function, (void *)&thread_ids[1], (void *)&use_lock);
  thread_create(thread_function, (void *)&thread_ids[2], (void *)&use_lock);
  
  // Wait for all threads to complete
  thread_join();
  thread_join();
  thread_join();
  
  printf(1, "\n==== Concurrent Execution Test (without locks) ====\n");
  
  // Reset execution order counter
  execution_order = 0;
  
  // Create three threads without lock
  thread_create(thread_function, (void *)&thread_ids[0], (void *)&no_lock);
  thread_create(thread_function, (void *)&thread_ids[1], (void *)&no_lock);
  thread_create(thread_function, (void *)&thread_ids[2], (void *)&no_lock);
  
  // Wait for all threads to complete
  thread_join();
  thread_join();
  thread_join();
  
  printf(1, "\nTest completed successfully!\n");
  exit();
}
