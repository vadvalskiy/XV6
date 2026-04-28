#include "types.h"
#include "stat.h"
#include "user.h"

volatile int shared = 0;

void
worker(void *arg)
{
    int *x = (int*)arg;

    printf(1, "thread: started, arg=%d\n", *x);
    shared = 1234;
    printf(1, "thread: shared set to %d\n", shared);

    exit();
}

int
main(int argc, char *argv[])
{
    void *stackbase;
    void *stack;
    void *joinedstack;
    int arg;
    int pid;
    int jpid;

    arg = 42;
    joinedstack = 0;

    stackbase = malloc(8192);
    if(stackbase == 0){
        printf(1, "threadtest: malloc failed\n");
        exit();
    }

    stack = (void*)(((uint)stackbase + 4095) & ~4095);

    printf(1, "main: before clone, shared=%d\n", shared);

    pid = clone(worker, &arg, stack);
    if(pid < 0){
        printf(1, "threadtest: clone failed\n");
        exit();
    }

    jpid = join(&joinedstack);
    if(jpid < 0){
        printf(1, "threadtest: join failed\n");
        exit();
    }

    printf(1, "main: join returned pid=%d\n", jpid);
    printf(1, "main: shared=%d\n", shared);
    printf(1, "main: joined stack=%p\n", joinedstack);

    free(stackbase);
    exit();
}