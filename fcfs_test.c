#include "types.h"
#include "user.h"

int main(int argc, char *argv[]) {
    int num_children = 5;
    int i;
    for(i = 0; i < num_children; i++) {
        int pid = fork();
        setpriority(3);
        if(pid < 0){
            printf(1, "Fork failed\n");
            exit();
        }
        if(pid == 0){
            // Child process
            printf(1, "Child %d started (PID: %d)\n", i, getpid());
            // Simulate some work with sleep
            sleep(100); // Adjust sleep duration as needed
            printf(1, "Child %d finished (PID: %d)\n", i, getpid());
            exit();
        }
        // Parent process continues to fork next child
    }

    // Parent waits for all children to finish
    for(i = 0; i < num_children; i++) {
        wait();
    }
    printf(1, "All children have finished.\n");
    exit();
}
