#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAXBUF 512

void tail(int fd) {
    char buf[MAXBUF];
    int n;
    int start = 0;
    int total = 0;
    
    // Read file into buffer in chunks
    while ((n = read(fd, buf + total, MAXBUF - total)) > 0) {
        total += n;
        if (total >= MAXBUF) {
            start = total - MAXBUF; // Keep only last MAXBUF bytes
            total = MAXBUF;
        }
    }
    
    if (n < 0) {
        printf(2, "tail: read error\n");
        return;
    }
    
    // Print last MAXBUF bytes
    for (int i = start; i < total; i++) {
        write(1, &buf[i], 1);
    }
}

int main(int argc, char *argv[]) {
    int fd;

    if(argc <= 1) {
        tail(0); // read from stdin
        exit();
    }

    for(int i = 1; i < argc; i++) {
        if((fd = open(argv[i], 0)) < 0){
            printf(2, "tail: cannot open %s\n", argv[i]);
            continue;
        }
        tail(fd);
        close(fd);
    }

    exit();
}
