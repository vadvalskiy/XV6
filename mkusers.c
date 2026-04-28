#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define ROLE_USER 0
#define ROLE_ADMIN 1

struct user {
  char username[16];
  unsigned int passhash;
  int role;
};

// Simple hash function for passwords.
unsigned int simple_hash(char *s) {
  unsigned int h = 5381;

  for(int i = 0; s[i]; i++){
    h = ((h << 5) + h) + s[i]; // h * 33 + c
  }

  return h;
}

int main(void)
{
  int fd = open("users", O_CREATE | O_WRONLY);

  struct user u;
  
  strcpy(u.username, "admin");
  u.passhash = simple_hash("admin");
  u.role = ROLE_ADMIN;
  write(fd, &u, sizeof(u));

  strcpy(u.username, "bob");
  u.passhash = simple_hash("1111");
  u.role = ROLE_USER;
  write(fd, &u, sizeof(u));

  close(fd);

  printf(1, "users file created\n");
  exit();
}