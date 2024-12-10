#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_PROCESSES 5

void busy_work() {
  for (int i = 0; i <= 100000000; i++){
    continue;
  }
  exit();
}

int main(void) {
  int i;

  for (i = 0; i < NUM_PROCESSES; i++) {
    int pid = fork();
    if (pid == 0) {
      // Child process: sleep
      busy_work();
    }
  }

  // Parent process: wait for all children
  for (i = 0; i < NUM_PROCESSES; i++) {
    wait();
  }

  exit();
}
