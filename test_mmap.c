#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>

int main() {
    const char *filename = "example.txt";

    // Open file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Get file size
    struct stat st;
    fstat(fd, &st);
    size_t filesize = st.st_size;

    // Map file into memory
    char *mapped = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Print file content from memory
    printf("File content:\n%.*s", (int)filesize, mapped);

    // Unmap the memory and close the file
    munmap(mapped, filesize);
    close(fd);

    return 0;
}
