#include "types.h"
#include "user.h"

int main(void)
{
    int pid1, pid2, pid3;

    // Process 1
    pid1 = fork();
    if (pid1 == 0) {
        // Child process 1
        setpriority(2);
        setburst(10);           // Set burst time for process 1
        setconfidence(80);      // Set confidence for process 1
        while (1) ;             // Infinite loop to simulate a long-running process
    }

    // Process 2
    pid2 = fork();
    if (pid2 == 0) {
        // Child process 2
        setpriority(2);
        setburst(8);            // Set burst time for process 2
        setconfidence(60);      // Set confidence for process 2
        while (1) ;             // Infinite loop to simulate a long-running process
    }

    // Process 3
    pid3 = fork();
    if (pid3 == 0) {
        // Child process 3
        setpriority(2);
        setburst(12);           // Set burst time for process 3
        setconfidence(90);      // Set confidence for process 3
        while (1) ;             // Infinite loop to simulate a long-running process
    }

    // Parent process waits for children (optional, for cleanup purposes)
    wait();
    wait();
    wait();

    exit();
}
