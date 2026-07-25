#include "types.h"
#include "stat.h"
#include "user.h"

#define EARLY 3
#define LATE 3
#define TOTAL (EARLY + LATE + 1)

static void
reader(int id, int late, int fd)
{
  int event = (late ? 70 : 10) + id;
  if(rw_acquire_read() < 0) exit();
  write(fd, &event, sizeof(event));
  sleep(late ? 5 : 35);
  if(rw_release_read() < 0) exit();
  exit();
}

int
main(int argc, char *argv[])
{
  int events[2], writer_ready[2], values[TOTAL];
  int i, j, pid, event, ready;
  int writer_index = -1;
  int failures = 0;

  if(rw_release_read() != -1 || rw_release_write() != -1) failures++;
  if(rw_acquire_read() < 0 || rw_acquire_read() != -1 ||
     rw_release_read() < 0 || rw_release_read() != -1) failures++;

  if(pipe(events) < 0 || pipe(writer_ready) < 0) exit();
  for(i = 0; i < EARLY; i++){
    pid = fork();
    if(pid == 0){
      close(events[0]); close(writer_ready[0]); close(writer_ready[1]);
      reader(i, 0, events[1]);
    }
  }

  // Wait until all early readers actually hold the lock.
  for(i = 0; i < EARLY; i++)
    if(read(events[0], &values[i], sizeof(values[i])) != sizeof(values[i])) failures++;

  pid = fork();
  if(pid == 0){
    close(events[0]); close(writer_ready[0]);
    ready = 1;
    write(writer_ready[1], &ready, sizeof(ready));
    close(writer_ready[1]);
    if(rw_acquire_write() < 0) exit();
    event = 50;
    write(events[1], &event, sizeof(event));
    sleep(10);
    if(rw_release_write() < 0) exit();
    exit();
  }
  close(writer_ready[1]);
  if(read(writer_ready[0], &ready, sizeof(ready)) != sizeof(ready)) failures++;
  close(writer_ready[0]);
  // Give the writer time to enter the kernel wait queue while early readers hold.
  sleep(2);

  for(i = 0; i < LATE; i++){
    pid = fork();
    if(pid == 0){ close(events[0]); reader(i, 1, events[1]); }
  }
  close(events[1]);
  for(i = 0; i < TOTAL; i++) wait();
  for(i = EARLY; i < TOTAL; i++){
    if(read(events[0], &values[i], sizeof(values[i])) != sizeof(values[i])){
      failures++; break;
    }
    if(values[i] == 50) writer_index = i;
  }
  close(events[0]);

  if(writer_index != EARLY) failures++;
  for(i = 0; i < EARLY; i++) if(values[i] < 10 || values[i] >= 20) failures++;
  for(j = writer_index + 1; j < TOTAL; j++) if(values[j] < 70) failures++;

  if(failures == 0)
    printf(1, "LAB4 RW TEST PASS\n");
  else
    printf(1, "LAB4 RW TEST FAIL: %d failure(s)\n", failures);
  exit();
}
