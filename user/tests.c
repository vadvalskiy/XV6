#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_CHILDREN 3
#define ITERATIONS 100000000

struct child {
    int pid;
    int tickets;
    int ticks;
};

int main(int argc, char *argv[]) {
    struct child children[NUM_CHILDREN];
    int ticket_counts[NUM_CHILDREN] = {10, 20, 30};
    int i;

    printf(1, "=== testlottery: starting ===\n");

    for(i = 0; i < NUM_CHILDREN; i++) {
        int pid = fork();
        if(pid < 0){
            printf(1, "fork failed\n");
            exit();
        } 
        if(pid == 0) {
            // child process
            settickets(ticket_counts[i]);

            // CPU-bound workload
            volatile int sink = 0;
            for(int j = 0; j < ITERATIONS; j++) {
                sink += j;
            }

            // Print ticks used (requires ticks in struct proc)
            // getpinfo() or direct syscall optional
            int t = getticks(); // if you implemented getticks() returning myproc()->ticks
            printf(1, "Child %d (PID %d, tickets %d) used %d ticks\n",
                   i+1, getpid(), ticket_counts[i], t);
            exit();
        } else {
            // parent stores child info
            children[i].pid = pid;
            children[i].tickets = ticket_counts[i];
        }
    }

    // Parent waits for all children
    for(i = 0; i < NUM_CHILDREN; i++){
        wait();
    }

    printf(1, "=== testlottery: all children done ===\n");
    exit();
}
