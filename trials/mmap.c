#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGESIZE 4096

void main(void)
{
    int fd;

    char *buff = mmap(0, PAGESIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANON|MAP_PRI);
    if(buff == MAP_FAILED) { printf("mmap error !\n"); return; }

    //char buff[PAGESIZE];
    //char *buff = (char *)malloc(PAGESIZE);

    memset(buff, 0x00, PAGESIZE);

    strcpy(buff, "Hello world");

    //memset(buff, 0x0a, PAGESIZE);

    printf("buff: %s\n", buff);

    if(munmap(buff, PAGESIZE)==-1) { printf("munmap error!\n"); }
}

