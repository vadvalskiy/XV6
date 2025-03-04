// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  // Open console in read-write mode
  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  // Duplicate file descriptors for stdin/stdout/stderr
  dup(0);
  dup(0);

  int attempts = 0;
  char uname[32], pword[32];

  while (attempts < 3) {
    printf(1, "Enter Username: ");
    gets(uname, sizeof(uname));
    // Remove trailing newline if present
    if(uname[strlen(uname)-1] == '\n')
      uname[strlen(uname)-1] = 0;

    if(strcmp(uname, USERNAME) == 0) {
      printf(1, "Enter Password: ");
      gets(pword, sizeof(pword));
      if(pword[strlen(pword)-1] == '\n')
        pword[strlen(pword)-1] = 0;

      if(strcmp(pword, PASSWORD) == 0) {
        printf(1, "Login successful\n");
        break;
      } else {
        printf(1, "Invalid password\n");
      }
    } else {
      printf(1, "Invalid username\n");
    }

    attempts++;
  }

  // If we used up all attempts, just hang or shutdown
  if (attempts == 3) {
    printf(1, "Maximum attempts reached. System halting.\n");
    for(;;) { } // Hang forever
  }

  // Now run the shell in a loop (like stock xv6 init)
  for(;;){
    printf(1, "init: starting sh\n");
    int pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      // Child: exec the shell
      char *argv[] = { "sh", 0 };
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    // Parent waits for shell to exit, then restarts it
    wait();
  }
}
