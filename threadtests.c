// Test threads

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "param.h"

#define CHECKMARK "\xE2\x9C\x85"
#define XMARK "\xE2\x9D\x8C"
#define REDQUESTIONMARK "\xE2\x9D\x93"
#define PARTYPOPPER "\xF0\x9F\x8E\x89"

#define TEST(test_func) (test_func)();                                                                                    \
                        printf(1, CHECKMARK " " #test_func "\n");

#define ASSERT(cond)    do{                                                                                               \
                          if(!(cond)){                                                                                    \
                            printf(2, XMARK " %s\n", __FUNCTION__);                                                       \
                            printf(2, REDQUESTIONMARK " ERROR in line %d - '%s' is false\n", __LINE__, #cond);            \
                            exit();                                                                                       \
                          }                                                                                               \
                        } while(0)

/**
 * Definitions for tests
 */
#define NORMAL_STACK_SIZE 1000
#define SECRET_VALUE1 0xdead
#define SECRET_VALUE2 0xbeef
#define MESSAGE_FILE "msg.txt"
#define MESSAGE "This is a message"
#define DIR1 "my_dir1"
#define DIR2 "my_dir2"

/**
 * Globals
 */
int g_secret;
int g_fd;
int g_tid;

/**
 * Functions for threads
 */
void*
empty(void)
{
  return 0;
}

void*
loop_forever(void)
{
  for(;;){}
}

void*
return_tid(void)
{
  int *tid;

  ASSERT((tid = malloc(sizeof(int))) > 0);

  *tid = gettid();

  return tid;
}

void*
change_secret_from_1_to_2(void)
{
  ASSERT(g_secret == SECRET_VALUE1);
  g_secret = SECRET_VALUE2;

  return 0;
}

void*
write_to_global_open_fd(void)
{
  ASSERT(write(g_fd, MESSAGE, sizeof(MESSAGE)) == sizeof(MESSAGE));
  return 0;
}

void*
chdir_to_dir1(void)
{
  ASSERT(chdir(DIR1) == 0);
  
  return 0;
}

void*
sleep_some_time(void)
{
  ASSERT(sleep(60) == 0);  
  return 0;
}

void*
join_tid(void)
{
  ASSERT(thread_join(g_tid, 0) == g_tid);  
  return 0;
}

/**
 * Tests
 */
void
verify_different_tids(void)
{  
  char stack1[NORMAL_STACK_SIZE];
  char stack2[NORMAL_STACK_SIZE];
  int tid1, tid2;
  int ourtid = gettid();

  ASSERT((tid1 = thread_create(empty, stack1, sizeof(stack1))) > 0);
  ASSERT((tid2 = thread_create(empty, stack2, sizeof(stack2))) > 0);

  ASSERT(tid1 != tid2);
  ASSERT(tid1 != ourtid);
  ASSERT(tid2 != ourtid);

  ASSERT(thread_join(tid1, 0) == tid1);
  ASSERT(thread_join(tid2, 0) == tid2);
}

void
illegal_thread_create(void)
{
  char stack[NORMAL_STACK_SIZE];

  ASSERT(thread_create(0, stack, sizeof(stack)) < 0);
  ASSERT(thread_create(empty, 0, sizeof(stack)) < 0);
  ASSERT(thread_create(empty, 0, 0) < 0);
  ASSERT(thread_create(0, 0, 0) < 0);
}

void
gettid_twice(void)
{
  ASSERT(gettid() == gettid());
}

void
kill_child_with_threads(void)
{
  char stack[NORMAL_STACK_SIZE];
  int pid = fork();
  ASSERT(pid >= 0);

  if(pid == 0){
    // create a thread that loops forever, and then loop forever
    ASSERT(thread_create(loop_forever, stack, sizeof(stack)));
    for(;;) {}
  } else{
    ASSERT(sleep(10) == 0); // Sleeping to make the child create the thread before we kill
    ASSERT(kill(pid) == 0);
  }

  ASSERT(wait() == pid);
}

void
return_tid_from_thread()
{
  char stack[NORMAL_STACK_SIZE];
  int tid, *rettid;

  ASSERT((tid = thread_create(return_tid, stack, sizeof(stack))) > 0);
  ASSERT(thread_join(tid, (void**)&rettid) == tid);
  ASSERT(*rettid == tid);

  free(rettid);
}

void
read_and_change_global_in_thread()
{
  char stack[NORMAL_STACK_SIZE];
  int tid;

  g_secret = SECRET_VALUE1;

  ASSERT((tid = thread_create(change_secret_from_1_to_2, stack, sizeof(stack))) > 0);
  ASSERT(thread_join(tid, 0) == tid);

  ASSERT(g_secret == SECRET_VALUE2);
}

void
open_global_fd_and_thread_write(void)
{
  char buf[sizeof(MESSAGE)];
  char stack[NORMAL_STACK_SIZE];
  int tid, fd;

  ASSERT((g_fd = open(MESSAGE_FILE, O_WRONLY|O_CREATE)) >= 0);
  ASSERT((tid = thread_create(write_to_global_open_fd, stack, sizeof(stack))) > 0);
  ASSERT(thread_join(tid, 0) == tid);

  ASSERT(close(g_fd) == 0);

  // Reading from the file to check if it was written as expected
  ASSERT((fd = open(MESSAGE_FILE, O_RDONLY)) >= 0);
  ASSERT(read(fd, buf, sizeof(MESSAGE)) == sizeof(MESSAGE));
  ASSERT(strcmp(buf, MESSAGE) == 0);
}

void
check_pid_overflow()
{
  int i, pid, ourpid;

  ourpid = getpid();

  for(i = 0; i < MAX_PID * 2; i++){
    pid = fork();    
    if(pid == 0)
      exit();

    ASSERT(pid > 0);    
    ASSERT(wait() == pid);
    ASSERT(pid != ourpid && pid != MIN_PID);
  }
}

void
check_tid_overflow()
{
  char stack[NORMAL_STACK_SIZE];
  int i, tid;
  int ourtid = gettid();

  ourtid = gettid();

  for(i = 0; i < MAX_TID * 2; i++){
    ASSERT((tid = thread_create(empty, stack, sizeof(stack))) > 0);
    ASSERT(thread_join(tid, 0) == tid);

    ASSERT(tid != ourtid && tid != MIN_TID);
  }
}

void
parent_child_different_tid()
{
  int parenttid = gettid();
  int pid = fork();
  ASSERT(pid >= 0);

  if(pid == 0){
    if(gettid() == parenttid){
      // If we failed the test, run forever in the child to mark that it didn't pass.
      for(;;) {}
    }
    exit();
  }

  ASSERT(wait() == pid);
}

void
max_threads(void)
{
  char stack[NORMAL_STACK_SIZE];
  int tids[NPROC];
  int n, tid;

  ASSERT(memset(tids, 0, NPROC));

  for(n=0; n<NPROC; n++){
    if((tid = thread_create(empty, stack, sizeof(stack))) < 0)
      break;
    tids[n] = tid;
  }

  ASSERT(n != NPROC);

  for(; n > 0; n--)
    ASSERT(thread_join(tids[n-1], 0) == tids[n-1]);
}

void
chdir_in_thread()
{
  char stack[NORMAL_STACK_SIZE];
  int tid, fd;

  ASSERT(chdir("/") >= 0);
  ASSERT(mkdir(DIR1) >= 0);
  ASSERT(mkdir(DIR1 "/" DIR2) >= 0);

  ASSERT((fd = open(DIR1, O_RDONLY)) > 0);
  ASSERT(close(fd) == 0);
  ASSERT(open(DIR2, O_RDONLY) < 0);

  ASSERT((tid = thread_create(chdir_to_dir1, stack, sizeof(stack))) > 0);
  ASSERT(thread_join(tid, 0) == tid);

  // Thread should have chdir to DIR1, so now it should succeed.
  ASSERT((fd = open(DIR1, O_RDONLY)) < 0);
  ASSERT((fd = open(DIR2, O_RDONLY)) > 0);
  ASSERT(close(fd) == 0);

  ASSERT(chdir("/") >= 0);
  ASSERT(unlink(DIR1 "/" DIR2) >= 0);
  ASSERT(unlink(DIR1) >= 0);
}

void
join_in_two_threads()
{

}

int
main(void)
{
  TEST(verify_different_tids);
  TEST(illegal_thread_create);
  TEST(gettid_twice);
  TEST(kill_child_with_threads);
  TEST(return_tid_from_thread);
  TEST(read_and_change_global_in_thread);
  TEST(open_global_fd_and_thread_write);
  TEST(check_pid_overflow);
  TEST(check_tid_overflow);
  TEST(parent_child_different_tid);
  TEST(max_threads);
  TEST(chdir_in_thread);
  TEST(join_in_two_threads);

  printf(1, PARTYPOPPER " All tests passed " PARTYPOPPER "\n");

  exit();
}