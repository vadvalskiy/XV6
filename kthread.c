#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "cflags.h"

// kernel_threads user library implementation 
#define KERNEL_STACK_ALLOC      (1)
#define SBRK_STACK_ALLOC        (2)

// change the macro to change the implementation of the library
#define LIB_IMPLEMENTATION      (KERNEL_STACK_ALLOC)

#define KERNEL_THREAD_STACK_SIZE      (4096)

#define BASE_ADDRESS(stack)     ((stack) + KERNEL_THREAD_STACK_SIZE)
#define START_ADDRESS(stack)    ((stack) - KERNEL_THREAD_STACK_SIZE)


// module contains userland threading library which creates
// threads using the underlying system calls like clone and join

// creates thread stack using heap memory allocation
int 
create_thread_stack(void **stack) 
{
 
    // kernel_thread library manages thread stack allocation
    #if LIB_IMPLEMENTATION == SBRK_STACK_ALLOC
   
    *stack = malloc(KERNEL_THREAD_STACK_SIZE);
    // malloc fails
    if(*stack == 0) {
        return -1;
    }

    // the stack address is the base address 
    *stack = BASE_ADDRESS(*stack);
    
    // kernel manages the thread stack allocation
    #else
        *stack = 0;
    #endif 

    // success 
    return 0;
}

// destory the stack allocated for thread 
void
destory_thread_stack(void **stack) 
{
    // kernel_thread library manages thread stack allocation
    #if LIB_IMPLEMENTATION == SBRK_STACK_ALLOC
    
    // deallocate memmory of thread stack
    free(START_ADDRESS(*stack));
    *stack = 0;
    
    #endif 

    return;
}


// creates the threads using clone system call implementation 
// returns 0 if successfully creates thread else it returns -1 
int 
thread_create(kernel_thread_t *kernel_thread, int func(void *args), void *args)
{
    kernel_thread->state = NEW;
    
    // allocate child stack 
    if(create_thread_stack(&(kernel_thread->tstack)) == -1) {
        return -1;
    }
    
    // cannot create thread 
    if((kernel_thread->tid = clone(func, kernel_thread->tstack, TFLAGS, args)) == -1) {
        destory_thread_stack(&(kernel_thread->tstack));
        kernel_thread->state = DEAD;
        return -1;
    }

    // thread is running 
    kernel_thread->state = RUNNING;
    return 0;
}

// join the thread in thread group
int 
kernel_thread_join(kernel_thread_t *kernel_thread) 
{
    // thread has already died 
    if(kernel_thread->state == DEAD) {
        return -1;
    }
    
    // join system call joining the thread 
    int jtid = join(kernel_thread->tid);

    // destory the stack allocated for the thread 
    destory_thread_stack(&(kernel_thread->tstack));

    kernel_thread->state = DEAD;
    return jtid;
}

// cancel thread in thread group
void
kernel_thread_cancel(kernel_thread_t *kernel_thread) {
    
    // thread has already died 
    if(kernel_thread->state == DEAD) {
        return;
    }
    
    // kill the thread execution 
    kill(kernel_thread->tid);

    // deallocate stack of thread exeuction 
    destory_thread_stack(&(kernel_thread->tstack));
    
    // thread has died 
    kernel_thread->state = DEAD;

    return;
}


// kernel_thread exit for the thread routine 
int 
kernel_thread_exit() 
{
    // exits handles dealloction of stack 
    exit();
}


