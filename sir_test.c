#include "types.h"
#include "user.h"

#define NUM_THREADS 2
#define NUM_INCREMENTS 100000 // 100k increments per thread

// Shared global variable
volatile int shared_counter = 0;

// Declare a lock
lock_t counter_lock;

// Worker function to increment the shared counter
void incrementer(void *arg1, void *arg2) {
    int thread_num = *(int*)arg1;
    printf(1, "Thread %d: Starting...\n", thread_num);
    
    for (int i = 0; i < NUM_INCREMENTS; i++) {
        // Acquire the lock before accessing shared data
        lock_acquire(&counter_lock);
        
        // Critical section - protected by the lock
        shared_counter++;
        
        // Release the lock
        lock_release(&counter_lock);
    }
    
    printf(1, "Thread %d: Finished (%d increments).\n", thread_num, NUM_INCREMENTS);
    exit();
}

int main(int argc, char *argv[]) {
    int args[NUM_THREADS];
    
    printf(1, "Main: Starting test with %d threads, %d increments each...\n",
           NUM_THREADS, NUM_INCREMENTS);
    
    // Initialize the lock
    lock_init(&counter_lock);
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = i + 1;
        thread_create(incrementer, &args[i], 0);
        printf(1, "Main: Created thread %d\n", args[i]);
    }
    
    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_join();
    }
    
    printf(1, "Main: All threads joined.\n");
    
    // Check final result
    int expected_value = NUM_THREADS * NUM_INCREMENTS;
    printf(1, "Main: Final counter value: %d\n", shared_counter);
    printf(1, "Main: Expected counter value: %d\n", expected_value);
   
    exit();
}
