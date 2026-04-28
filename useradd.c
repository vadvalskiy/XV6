#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define ROLE_USER 0
#define ROLE_ADMIN 1
#define MAX 100

struct user {
  char username[16];
  unsigned int passhash;
  int role;
};

unsigned int simple_hash(char *s) {
  unsigned int h = 5381;

  for(int i = 0; s[i]; i++){
    h = ((h << 5) + h) + s[i]; // h * 33 + c
  }

  return h;
}

int main(int argc, char *argv[])
{
  if(argc < 3){
    printf(1, "usage: useradd name pass\n");
    exit();
  }

  struct user users[MAX];
  int count = 0;

  int fd = open("users", O_RDONLY);
  if(fd >= 0){
    while(read(fd, &users[count], sizeof(struct user)) == sizeof(struct user)){
      count++;
    }
    close(fd);
  }

  strcpy(users[count].username, argv[1]);
  users[count].passhash = simple_hash(argv[2]);
  users[count].role = ROLE_USER;
  count++;

  fd = open("users", O_CREATE | O_WRONLY);
  for(int i = 0; i < count; i++){
    write(fd, &users[i], sizeof(struct user));
  }
  close(fd);

  printf(1, "user created\n");
  exit();
}