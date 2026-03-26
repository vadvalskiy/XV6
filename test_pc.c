#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

#define BSIZE 5
#define NITEMS 10

static int buffer[BSIZE];
static int count = 0;
static int in = 0;
static int out = 0;
static umutex_t m;

void
producer(void *arg)
{
  int i;
  int value;

  for(i = 0; i < NITEMS; i++){
    value = i + 1;

    while(1){
      mutex_lock(&m);

      if(count < BSIZE){
        buffer[in] = value;
        in = (in + 1) % BSIZE;
        count++;

        printf(1, "producer: produced %d, count=%d\n", value, count);

        mutex_unlock(&m);
        break;
      }

      mutex_unlock(&m);
      thread_yield();
    }

    thread_yield();
  }
}

void
consumer(void *arg)
{
  int i;
  int value;

  for(i = 0; i < NITEMS; i++){
    while(1){
      mutex_lock(&m);

      if(count > 0){
        value = buffer[out];
        out = (out + 1) % BSIZE;
        count--;

        printf(1, "consumer: consumed %d, count=%d\n", value, count);

        mutex_unlock(&m);
        break;
      }

      mutex_unlock(&m);
      thread_yield();
    }

    thread_yield();
  }
}

int
main(int argc, char *argv[])
{
  int prod_tid;
  int cons_tid;

  thread_init();
  mutex_init(&m);

  prod_tid = thread_create(producer, 0);
  cons_tid = thread_create(consumer, 0);

  if(prod_tid < 0 || cons_tid < 0){
    printf(1, "thread_create failed\n");
    exit();
  }

  thread_join(prod_tid);
  thread_join(cons_tid);

  printf(1, "done: producer/consumer finished successfully\n");
  exit();
}