#include "types.h"
#include "stat.h"
#include "user.h"

void producer();
void consumer();

int buffer_count = 0;

void producer() {
  for(int i = 0; i < 10; i++) {
    while(buffer_count >= 5) thread_yield();
    buffer_count++;
    printf(1, "Producer: count %d\n", buffer_count);
    thread_yield();
  }
  exit();
}

void consumer() {
  for(int i = 0; i < 10; i++) {
    while(buffer_count <= 0) thread_yield();
    buffer_count--;
    printf(1, "Consumer: count %d\n", buffer_count);
    thread_yield();
  }
  exit();
}

int main() {
  thread_init();
  thread_create(producer);
  thread_create(consumer);
  
  while(1) {
    thread_yield();
  }
  return 0;
}