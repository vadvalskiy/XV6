#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"


//printing purposes

//I copied how terminal formats file names in tree command
char*
fmtname(char *path)
{
    static char buf[DIRSIZ+1];//adding 1 byte for null terminator \0
    char *p;
    //we find the first part of the name after the last slash
    //when detected, for loop ends and we have p index saved
    for(p = path + strlen(path); p >= path && *p != '/'; p--)
    ;

    //since when *p == '/' loop ends but decrementing still happened
    //we just reverting that decrement
    p++;

    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0;
    return buf;
}

void
print_indent(int depth)
{
    int i;
    for(i = 0; i < depth; i++)
        printf(1, "|   ");
}
//

//recursion approach
void
tree(char *path, int depth)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st; 

    if((fd = open(path, O_RDONLY)) < 0){
        printf(2, "tree: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0){
        printf(2, "tree: cannot stat %s\n", path);
        close(fd);
        return;
    }

    print_indent(depth);
    printf(1, "|-- %s\n", fmtname(path));

    //if type is not a directory then return
    //only activate recursion if directory
    if(st.type != T_DIR){
        close(fd);
        return;
    }

    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
        printf(2, "tree: path too long\n");
        close(fd);
        return;
    }

    //building child path 
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        //unused, ignore
        if(de.inum == 0)
        continue;
        //skipping . and .. directories to avoid infinite recursion
        if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;

        //appending child name to path where we attach it after the slash
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;//null terminate
        tree(buf, depth + 1);
    }

    close(fd);
}

int
main(int argc, char *argv[])
{
    if(argc == 1)
        tree(".", 0);
    else if(argc == 2)
        tree(argv[1], 0);
    else{
        printf(2, "usage: tree [path]\n");
        exit();
    }
    exit();
}