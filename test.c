#include "types.h"
#include "user.h"
#include "stat.h"

int main()
{
    // --- test ps start ---
    printf(1, "====test start====");
    int pid = fork();
    extern int ticks;
    if(pid == 0){
        int a = 0;
        for(int i = 0;;i++){
            a++;
        }
        exit();
    }
    else {
        int a = 0;
        setnice(4, 0);
        setnice(3, 5);

        for(int i = 0;;i++){
            a++;
        }
        wait();
    }
    // --- test ps end ---
    // what up
    exit();
}