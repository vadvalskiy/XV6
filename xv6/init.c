// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    
    printf(1, "\n\n");
    printf(1, "                                                 \\,`/ /      \n");
    printf(1, "                  _  _  _  _   ___              _)..  `_      \n");
    printf(1, "                 ( \\/ )/ )( \\ / __)            ( __  -\\    \n");
    printf(1, "                  )  ( \\ \\/ /(  _ \\                '`.     \n");
    printf(1, "                 (_/\\_) \\__/  \\___/               ( \\>_-_,\n");
    printf(1, "                                                  _||_ ~-/    \n");
    printf(1, "\n\n\n");
    printf(1, "    Modified by group:\n");
    printf(1, "    Meraj Rastegar 810102576\n");
    printf(1, "    Ali Sadeghi 810102471\n");
    printf(1, "    Meraj PourHosseiny 810102420\n\n");

    printf(1, "init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}
