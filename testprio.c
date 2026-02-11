#include "types.h"
#include "user.h"

int main(void) {
  int pid = getpid();
  
  printf(1, "Testing setpriority...\n");
  
  if (setpriority(pid, 0) == 0)
    printf(1, "Set to HIGH (0): PASS\n");
  else
    printf(1, "Set to HIGH (0): FAIL\n");
    
  if (setpriority(pid, 1) == 0)
    printf(1, "Set to NORMAL (1): PASS\n");
  else
    printf(1, "Set to NORMAL (1): FAIL\n");
    
  if (setpriority(pid, 2) == 0)
    printf(1, "Set to LOW (2): PASS\n");
  else
    printf(1, "Set to LOW (2): FAIL\n");
    
  if (setpriority(pid, 3) == -1)
    printf(1, "Invalid priority 3: PASS\n");
  else
    printf(1, "Invalid priority 3: FAIL\n");
    
  if (setpriority(9999, 1) == -1)
    printf(1, "Invalid PID 9999: PASS\n");
  else
    printf(1, "Invalid PID 9999: FAIL\n");
    
  printf(1, "Tests done.\n");
  exit();
}