#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

int match(char *pattern, char *name){
  if(*pattern == 0 && *name == 0)
    return 1;

  if(*pattern == '*'){
    return match(pattern+1, name) || (*name && match(pattern, name+1));
  }

  if(*pattern == *name)
    return match(pattern+1, name+1);

  return 0;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

// This implementation of ls with wildcard support was added and was not in the original xv6 codebase.
// this implementation only supports files in current directory.
void ls_wildcard(char *pattern) {
  int fd;
  struct dirent de;
  struct stat st;
  char buf[512], *p;

  if((fd = open(".", 0)) < 0){
    printf(2, "ls: cannot open .\n");
    return;
  }

  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;

    if(match(pattern, de.name)){
      strcpy(buf, ".");
      p = buf + strlen(buf);
      *p++ = '/';
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }

      printf(1, "%s %d %d %d\n",
        fmtname(buf), st.type, st.ino, st.size);
    }
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit();
  }
  for(i=1; i<argc; i++){
    // Check if the argument contains a wildcard character
    // this implementation was added and was not in the original xv6 codebase.
    // Debug: printf(1, "arg = %s\n", argv[i]);
    if(strchr(argv[i], '*')){
      ls_wildcard(argv[i]);
    } else {
      ls(argv[i]);
    }
  }
  exit();
}
