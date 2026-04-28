#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

struct user {
  char username[16];
  unsigned int passhash;
  int role;
};

int main(int argc, char *argv[])
{
  if(argc < 2){
    printf(1, "usage: deluser name\n");
    exit();
  }

  if(strcmp(argv[1], "admin") == 0){
    printf(1, "cannot delete admin user\n");
    exit();
  }

  int fd = open("users", O_RDWR);
  int newfd = open("users.tmp", O_WRONLY|O_CREATE);

  struct user u;

  while(read(fd, &u, sizeof(u)) == sizeof(u)){
    if(strcmp(u.username, argv[1]) != 0){
      write(newfd, &u, sizeof(u));
    }
  }

  close(fd);
  close(newfd);

  unlink("/users");
  
  fd = open("/users", O_CREATE | O_WRONLY);
  int tmp = open("/users.tmp", O_RDONLY);

  while(read(tmp, &u, sizeof(u)) == sizeof(u)){
    write(fd, &u, sizeof(u));
  }

  close(fd);
  close(tmp);
  unlink("/users.tmp");

  printf(1, "user deleted\n");
  exit();
}