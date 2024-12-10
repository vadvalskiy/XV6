#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_OF_PROCESSES 60
#define job_ITERATIONS 10000000

void job(int id) 
{
    volatile long long x = 0;
    for (long long i = 0; i < job_ITERATIONS; i++)
        x += i;
}

static unsigned int seed = 12345;

int rand()
{
    seed = (1103515245 * seed + 12345) % 2147483648;
    return seed;
}

int main(void) 
{
    int pid;
    for (int i = 0; i < NUM_OF_PROCESSES; i++) 
    {
        pid = fork();
        // if (i <= 20){
        //     setpriority(1);
        // }else if ( 20 < i && i <=40){
        //     setpriority(2);
        //     setburst(rand()%20);
        //     setconfidence(rand()%100);
        // }else {
        //     setpriority(3);
        // }
        if (pid < 0) 
        {
            printf(1, "Fork failed\n");
            exit();
        } 
        else if (pid == 0) 
        {
            // Child process
            job(i);
            exit();
        }
    }
    
    while (wait() > 0);
    
    printf(1, "All processes done\n\n");
    exit();
}
