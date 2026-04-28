//quick requirement outline
//I use fd1 fd2 to open file
//I make read line function to return line length
//comparison in main thru while loop

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
read_line(int fd, char *buf, int max)
{
    int i = 0;
    char temp;
    while(i < max - 1){
        int x = read(fd, &temp, 1);
        if(x <= 0)
        break;

        buf[i++] = temp;
        if(temp == '\n')
        break;
    }
    buf[i] = 0;
    return i; // length
}

int 
main(int argc, char *argv[])
{
    //bug fix.... added argument counter
    //without it you would be able to diff file1 without providing file2
    //so if not 3 counter, exit
    if(argc != 3){
        printf(2, "usage: diff file1 file2\n");
        exit();
    }

    //if we wanna allow  more than 2 files then we could make condition (argc < 3)
    //and then we need to figure out how to handle more 2files
    //which will definitely require helper functions

    int fd1 = open(argv[1], O_RDONLY);
    int fd2 = open(argv[2], O_RDONLY);
    if(fd1 < 0 || fd2 < 0){
        printf(2, "diff: cannot open file\n");
        exit();
    }

    char line1[100], line2[100];
    int n1, n2;
    int line = 1;
    int diff_found = 0;

    //infinite loop until break
    //might not be secure in the case of irregular or malicious files
    //but I do not know how to do it so it can detect irregularties
    //so I will stick to normal logic 
    while(1){
        n1 = read_line(fd1, line1, sizeof(line1));
        n2 = read_line(fd2, line2, sizeof(line2));
        
        if(n1 == 0 && n2 == 0) break;

        if(strcmp(line1, line2) != 0){
        printf(1, "Line %d differs:\n", line);

        printf(1, "file1: %s", line1);
        if(n1 == 0 || line1[n1-1] != '\n') printf(1, "\n");//for cleaner format

        printf(1, "file2: %s", line2);
        if(n2 == 0 || line2[n2-1] != '\n') printf(1, "\n");//for cleaner formatting
        diff_found = 1;
        }
        line++;
    }

    if(!diff_found)
        printf(1, "Files are identical\n");

    close(fd1);
    close(fd2);

    exit();
}