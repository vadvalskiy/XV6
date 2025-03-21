// // void *mmap(void *addr, size_t length, int prot, int flags,int fd, off_t offset);
// #include "types.h"
// #include "stat.h"
// #include "user.h"
// #include "fs.h"

// int main(int argc, char *argv[]){
//     void *addr = (void *) 0x400000;  // Example address
//     int length = 4096;               // 4 KB
//     int prot = 1;                    // Example: PROT_READ
//     int flags = 2;                    // Example: MAP_PRIVATE
//     int fd = -1;                      // No file (anonymous mapping)
//     int offset = 0;                   // Offset in file

//     void *result = mmap(addr, length, prot, flags, fd, offset);
//     if (result == (void *) -1) {
//         printf(1,"mmap failed\n");
//     } else {
//         printf(1,"mmap returned: %p\n", result);
//     }
//     munmap();
//     exit();
// }

#include "user.h"
#include "fcntl.h"

int main() {
    int len = 4096; // 1 page size
    void *addr = (void*)6000; // Let mmap choose the address

    void *result = mmap(addr, len, 0, 0, -1, 0);
    if (result == (void*)-1) {
        printf(1,"mmap failed\n");
        exit();
    }

    printf(1,"mmap success! Address: %p\n", result);

    // Write and read to test mapping
    char *test_mem = (char*) result;
    test_mem[0] = 'A';  // Store a value
    printf(1,"Stored: %c\n", test_mem[0]); // Check if stored correctly

    exit();
}
