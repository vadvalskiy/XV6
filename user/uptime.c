#include "kernel/types.h"
#include "user/user.h"

int main(void)
{
    fprintf(1,"ticks:%d\n",uptime());
    exit(0);
}