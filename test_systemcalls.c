#include "types.h"
#include "stat.h"
#include "user.h"


void stall(int id) 
{
    volatile long long x = 0;
    for (int i = 0; i < 100000000; i++)
    {
        if (i % 10000000 == 0){
            printinfo();
        }
        x += i;
    }
}

int main(void) 
{
    int pid;
    for (int i = 0; i < 10; i++) 
    {
        pid = fork();
        if (pid == 0) 
        {
            if (i < 5) 
            {
                printf(1, "Set queue called for process: %d\n" , getpid());
                setpriority(2);
            } 
            if (i < 5)
            {
                printf(1, "Set burst confidence called for process: %d\n" , getpid());
                setburst(1);
                setconfidence(70);
            }
            // Child process
            stall(getpid());
            exit();
        }


    }
    while (wait() > 0);
    
    printf(1, "All processes done\n\n");
    exit();
}
