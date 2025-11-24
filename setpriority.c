#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if(argc != 3){
        printf(1, "Usage: %s <pid> <priority>\n", argv[0]);
        exit();
    }

    int pid_in = atoi(argv[1]);
    int pr_in  = atoi(argv[2]);

    if(pr_in < 1 || pr_in > 64){
        printf(1, "Invalid priority value.\n");
        exit();
    }

    int old_pr = getpriority(pid_in);
    int old_pr_syscall = setpriority(pid_in, pr_in);

    if(old_pr != old_pr_syscall){
        printf(1, "Error: setpriority did not return the old priority.\n");
        exit();
    }

    int new_pr = getpriority(pid_in);

    if(new_pr != pr_in){
        printf(1, "Error: priority was not updated.\n");
        exit();
    }

    printf(1, "Old priority: %d\n", old_pr);
    printf(1, "New priority: %d\n", new_pr);

    exit();
}
