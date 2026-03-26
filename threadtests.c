#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "cflags.h"

#define TSTACK_SIZE     (4096)

#define eprintf(fmt, ...)                                                   \
    printf(1, fmt ": FAILED"                                                \
              "\nfile = %s, line number = %d, in function = %s()\n"         \
               ##__VA_ARGS__, __FILE__, __LINE__, __func__);                \
    exit();                                                                 \

#define sprintf(fmt, ...)                                                   \
    printf(1, fmt " : PASSED\n");                                           \


// all functions provide test cases for the xv6 kernel thread implementation 
// test cases are intended to produce appropriate results which can be verified.

int global_var;     // one global variable for all functions to test 

// ===========================================================================
// =========================== SYSTEM CALLS ==================================
// ===========================================================================

typedef struct sortargs {
  int *arr, start, end;
} sortargs;

// merge routine needed for merging two sorted arrays
void 
merge(int *arr, int start, int mid, int end) 
{
  int n1 = mid - start + 1;
  int n2 = end - mid;
  int left[n1 + 1], right[n2 + 1];
  for(int i = 0; i < n1; i++) {
    left[i] = arr[start + i];
  }
  for(int i = 0; i < n2; i++) {
    right[i] = arr[mid + i + 1];
  }
  left[n1] = right[n2] = (int)10e9;
  int i = 0, j = 0;
  for(int k = start; k <= end; k++) {
    if(left[i] <= right[j]) {
      arr[k] = left[i];
      i++;
    } else {
      arr[k] = right[j];
      j++;
    }
  }
  return;
}

// merge sort for sorting two arrays
void 
mergesort(int *arr, int start, int end) 
{
  if(start < end) {
    int mid = start + (end - start) / 2;
    mergesort(arr, start, mid);
    mergesort(arr, mid + 1, end);
    merge(arr, start, mid, end);
  }
}

// thread routine calls merge sort for sorting
int 
sort(void *args) 
{
  sortargs *ptr = (sortargs *)args; 
  mergesort(ptr->arr, ptr->start, ptr->end);
  exit();
}


// does clone system call for creating threads and waits using join system 
// creates two threads and waits for concurrent execution of merge sort
// TEST CASE : checks virtual address space is shared (heap and text/data)
//           : checks for threads being executed concurrently (as if were a process)
//           : check for join system call which blocks/suspends the thread
int 
clone_join_test() 
{
  void *cstack1, *cstack2;
  int *arr;
  int n = 100, left_tid, right_tid;
  sortargs left_args, right_args;

  arr = (int *)malloc(sizeof(int) * n);
  if(!arr) {
    eprintf("malloc failed");
  }
  // creating a reverse sorted array
  for(int i = 0; i < n; i++) {
    arr[i] = n - i;             
  }

  // arguments for thread execution 
  left_args.arr = right_args.arr = arr;
  left_args.start = 0;
  left_args.end = n / 2;
  right_args.start = (n / 2) + 1;
  right_args.end = n - 1;
  
  // stacks allocated for execution
  cstack1 = malloc(TSTACK_SIZE);
  cstack2 = malloc(TSTACK_SIZE);
  if(!cstack1 || !cstack2) {
    eprintf("malloc failed"); 
  }

  // creating threads for sorting concurrently 
  left_tid  = clone(sort, cstack1 + TSTACK_SIZE, TFLAGS, &left_args);
  right_tid = clone(sort, cstack2 + TSTACK_SIZE, TFLAGS, &right_args);
  
  join(left_tid);         // wait for left array to be sorted
  join(right_tid);        // wait for right array to be sorted
  
  // merge the sorted arrays 
  merge(arr, 0, n / 2, n - 1);
  
  // verifying if the array is sorted 
  for(int i = 0; i < n; i++) {
    if(arr[i] != i + 1) {
      eprintf("clone join test");
    }
  }
  sprintf("clone join test");

  free(arr);
  free(cstack1);
  free(cstack2);

  // success
  return 0;
}

// ===========================================================================

#define BAD_ADDRESS         ((void *)0xfffffff)

// although the function exits in thread address space
// the function never gets called since arguments passed 
// to function are invalid and clone system call fails
int 
not_arguments(void *args)
{
  int *ptr = (int *)args;
  *ptr = *ptr + 10;
  exit();
}

// wrong ways to call clone and join system calls test 
int 
wrong_syscall_test()
{   
  int tid, temp = 0;
  
  // passing invalid arguments which are not in address space
  tid = clone(not_arguments, 0, TFLAGS, BAD_ADDRESS);
  if(tid != -1){
    eprintf("wrong system call clone arguments");
  }
  
  // passing invalid function pointer address 
  tid = clone(BAD_ADDRESS, 0, TFLAGS, 0);
  if(tid != -1){
    eprintf("wrong system call clone function pointer");
  }
     
  // passing invalid stack child address 
  tid = clone(not_arguments, BAD_ADDRESS, TFLAGS, &temp);
  if(tid != -1){
    eprintf("wrong system call clone stack address");
  }

  // invalid flag passing 
  

  // join system call for any random thread id
  tid = 1234;
  if(join(tid) != -1){
    eprintf("wrong system call join random thread id");
  }

  // join system call for group leader (thread group leader has tid = -1)
  tid = -1;
  if(join(tid) != -1){
    eprintf("wrong system call join random thread id");
  }
  
  sprintf("wrong system call clone and join");
  // success
  return 0;
}

// ===========================================================================

#define MAX_ITERATIONS   (10000)
#define MAX_THREAD_POOL  (5)

int
incr_global(void *args) 
{
  for(int i = 0; i < MAX_ITERATIONS; i++) {
    global_var++;
  }
  exit();
}

// the clone and join system calls "without passing child stack parameter"
// the kernel allocates pages for stack, along with taking care of guard page
// kernel basically extends/grows the virtual address space of shared memory 
// TEST CASE : check if kernel allocates stack 
//           : creating thread pools for execution
int 
kernel_clone_stack_alloc() 
{
  // thread pool for storing thread ids
  int thread_pool[MAX_THREAD_POOL];
  // initializing global variables 
  global_var = 0;
  
  // create threads and execution begins concurrently 
  for(int i = 0; i < MAX_THREAD_POOL; i++) {
    thread_pool[i] = clone(incr_global, 0, TFLAGS, 0);
  }
  // join all the threads i.e. wait for its execution
  for(int i = 0; i < MAX_THREAD_POOL; i++) {
    join(thread_pool[i]);
  }
  if(global_var == MAX_THREAD_POOL * MAX_ITERATIONS) {
    sprintf("kernel clone stack allocation");
  } else {
    eprintf("kernel clone stack allocation");
  }
  // sucess
  return 0;
}

// ===========================================================================

#define FORK_TEST_FILE          "fork_test.txt"
#define FORK_STR                "foobarbaz"
#define FORK_STR_LEN            (9) 
#define FORK_SECRET             (196)
    
int fork_func_id, not_fork_func_id;

// thread simply sleeps 
int 
not_fork_func(void *agrs) 
{
  sleep(50);
  exit();
}

// creates child process and waits for it's execution 
int 
fork_func(void *agrs) 
{
  int pid, wpid, fd;
  char buf[FORK_STR_LEN + 1];
  
  // create identical child process  
  pid = fork();
  
  if(pid == -1){
    eprintf("fork test cannot make system call fork");
  }

  // child does write system call to make change in file system.
  if(pid == 0){
    
    fd = open(FORK_TEST_FILE, O_RDWR | O_CREATE);
    write(fd, FORK_STR, FORK_STR_LEN);
    close(fd);
    
    // child process doesn't have other thread in address space 
    // join must fail since thread doens't belong to group
    if(join(not_fork_func_id) != -1){
      eprintf("fork test failed join should not happend");
    }

    exit();
  }

  // thread waits for child process to exit
  wpid = wait(); 
  if(wpid == pid){
    
    // reads the file modified by the child process 
    fd = open(FORK_TEST_FILE, O_RDONLY);
    read(fd, buf, FORK_STR_LEN);
    buf[FORK_STR_LEN] = '\0';

    // compare the content insider file
    if(strcmp(buf, FORK_STR) == 0) {
      sprintf("fork test"); 
    } else {
      eprintf("fork test child process not working correctly"); 
    }
    close(fd);

  } else {
    eprintf("fork test wait and not working");
  }
  exit();
}

// thread making fork system call, creates new process with the only thread 
// executing for newly created process will be thread which called fork.
// TEST CASE : any thread can create new process using fork (with only one identical thread )
//           : any thread can wait for the child process 
//           : others thread apart for thread calling fork are never duplicated 
int 
fork_test() 
{
  // creates thread one for executing fork and one for increamenting global variable
  not_fork_func_id = clone(not_fork_func, 0, TFLAGS, 0);
  fork_func_id = clone(fork_func, 0, TFLAGS, 0);
  
  // join thread 
  join(fork_func_id);
  join(not_fork_func_id);

  // success 
  return 0;
}

// ===========================================================================

// ===========================================================================
// ===========================================================================
// ============================ KERNEL_THREAD LIBRARY ==============================
// ===========================================================================

// ===========================================================================
#define N                   (6)
#define M                   (2000000)
#define P                   (10)

#define MAGIC_MULTIPLE      (10)

typedef struct matrixargs{
  int **a, **b, **c;  // matrix a, b, and c
  int row;            // i is row of matrix a
  int col;            // j is column of matrix b
  int m;              // num of rows in a = num of columns in b
} matrixargs;

void
print_matrix(int **mat, int n, int m) 
{
  for(int i = 0; i < n; i++){
    for(int j = 0; j < m; j++){
      printf(1, "%d ", mat[i][j]);
    }
    printf(1, "\n");
  }
  printf(1, "\n");
}

int **
get_matrix(int n, int m) {
  int **mat = (int **)malloc(sizeof(int *) * n);
  if(mat == 0){
    eprintf("malloc failed");
  }
  for(int i = 0; i < n; i++) {
    mat[i] = (int *)malloc(sizeof(int) *m);
    if(mat[i] == 0){
      eprintf("malloc failed");
    }
  }
  // initialize all the elements in matrix to 1
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < m; j++) {
      mat[i][j] = 1;
    }
  }
  return mat;
}

void 
free_matrix(int **arr, int n, int m) {
  for(int i = 0; i < n; i++) {
    free(arr[i]);
  }
  free(arr);
}

int
multiply_row_col(void *args)
{
  matrixargs *ptr = (matrixargs *)args;
  int i, j, m;
  i = ptr->row, j = ptr->col;
  m = ptr->m;

  ptr->c[i][j] = 0;
  for(int k = 0; k < m; k++){
    ptr->c[i][j] += ptr->a[i][k] * ptr->b[k][j];
  }
  kernel_thread_exit();
}


// multhreading test gives a stress test for the kernel_thread library which
// implements one to one mapping. The threading library is used for 
// doing matrix multiplcation (C = A x B) by creating some 60 threads
int 
kernel_thread_lib_multithreading_test()
{
  printf(1, "STRESS TEST 2 : \n");

  int **arr = get_matrix(N, M);
  int **brr = get_matrix(M, P);
  int **crr = get_matrix(N, P);

  // arguments to be passed for matrix multiplication
  matrixargs args[N * P], temp; 
  kernel_thread_t kernel_thread_pool[N * P];

  // matrices 
  temp.a = arr;
  temp.b = brr;
  temp.c = crr;
  temp.m = M;
  
  // matrix multiplication using kernel_threadlibrary
  for(int i = 0; i < N; i++){
    for(int j = 0; j < P; j++){
      args[i * P + j] = temp;
      args[i * P + j].row = i;
      args[i * P + j].col = j;
      if(thread_create(kernel_thread_pool + i * P + j, multiply_row_col,
                        args + i * P + j) == -1){
        eprintf("multithreading test"); 
      }
    }
  }
  
  // waiting for all threads to complete 
  for(int i = 0; i < N * P; i++){
    if(kernel_thread_join(kernel_thread_pool + i)== -1){
      eprintf("multithreading test"); 
    }
  }

  // verifying if the matrix multiplication is correct 
  for(int i = 0; i < N; i++){
    for(int j = 0; j < P; j++){
      if(crr[i][j] != M){
        eprintf("multithreading test"); 
      }
    }
  }
  
  free_matrix(arr, N, M);
  free_matrix(brr, M, P);
  free_matrix(crr, N, P);

  sprintf("multithreading test");
  // success
  return 0;
}


// ===========================================================================

char *thread_str1 = "abcefg\n";
char *thread_str2 = "xyzlmn\n";

#define THREAD_STR_LEN      (7)

int tfd;
semaphore sem;

int
filewrite_func1(void *args)
{
  wait_sem(&sem);    
  for(int i = 0; i < THREAD_STR_LEN; i++) {
    write(tfd, &thread_str1[i], 1);
    sleep(10);
  }
  signal_sem(&sem);    
  kernel_thread_exit();
}

int
filewrite_func2(void *agrs)
{
  wait_sem(&sem);    
  for(int i = 0; i < THREAD_STR_LEN; i++) {
    write(tfd, &thread_str2[i], 1);
    sleep(10);
  }
  signal_sem(&sem);    
  kernel_thread_exit();
}

// the semaphore implementation issues synchronization among threads
// concurrently try to update and modify a shared resource 
// TESTCASE : semaphore ensures mutual execlusion among threads modifying data
int 
kernel_thread_semaphore_test() 
{
  kernel_thread_t th1, th2;
  char str[128];

  // binary semaphore mutex 
  init_sem(&sem, 1);

  tfd = open("sem.txt", O_RDWR | O_CREATE); 
  if(tfd == -1){
    eprintf("cannot open file");
  }

  thread_create(&th1, filewrite_func1, 0);
  thread_create(&th2, filewrite_func2, 0);
      
  kernel_thread_join(&th1);
  kernel_thread_join(&th2);

  close(tfd);
      
  // verifying if synchronization order
  tfd = open("sem.txt", O_RDONLY); 
  
  // first str1 should be written 
  read(tfd, str, THREAD_STR_LEN);
  str[THREAD_STR_LEN] = '\0';
  if(strcmp(str, thread_str1) != 0 && strcmp(str, thread_str2) != 0){
    eprintf("semaphore test");
  }

  // second str2 should be written 
  read(tfd, str, THREAD_STR_LEN);
  str[THREAD_STR_LEN] = '\0';
  if(strcmp(str, thread_str2) != 0 && strcmp(str, thread_str1) != 0){
    eprintf("semaphore test");
  }
  
  close(tfd);

  sprintf("semaphore test");
  // success 
  return 0;
}

// ===========================================================================

#define VAL1        (10)
#define VAL2        (20)
#define MAX_SIZE    (10)

int race_arr[10], race_index;

// userland spin lock 
struct mylock s;

// updates the global array 
int 
update_func1(void *args) 
{
  lock_acquire(&s);
  for(int i = 0; i < MAX_SIZE / 2; i++) {
    race_arr[race_index] = VAL1; 
    sleep(10);
    race_index++;
  }
  lock_release(&s);
  exit();
}

// updates the global array 
int 
update_func2(void *args) 
{
  lock_acquire(&s);
  for(int i = 0; i < MAX_SIZE / 2; i++) {
    race_arr[race_index] = VAL2; 
    sleep(10);
    race_index++;
  }
  lock_release(&s);
  exit();
}

// since we cannot user xv6 provided spinlock which is meant for xv6 kernel
// userland spin lock code was written and function test spin lock functionality 
int 
kernel_thread_uspinlock_test()
{
  kernel_thread_t th1, th2;
  lock_init(&s);
  
  thread_create(&th1, update_func1, 0);
  thread_create(&th2, update_func2, 0);
  
  kernel_thread_join(&th1);
  kernel_thread_join(&th2);
  
  // first half of the array must have same value either 1 or 2
  // similarly for the second half of the array values.
  for(int i = 0; i < MAX_SIZE / 2; i++){
    if(race_arr[0] != race_arr[i]){
      eprintf("spinlock test");
    }
  }
  for(int i = MAX_SIZE / 2; i < MAX_SIZE; i++) {
    if(race_arr[MAX_SIZE / 2] != race_arr[i]) {
      eprintf("spinlock test");
    }
  }
  
  sprintf("spinlock test");
  exit();
}

// ===========================================================================

// ===========================================================================

int
main(int argc, char *argv[])
{
    
  // SYSTEM CALL TESTS 
  
  clone_join_test();                  // simple clone and join system call
  wrong_syscall_test();               // wrong ways to call clone and join
  kernel_clone_stack_alloc();         // kernel allocating thread execution stack 
  fork_test();                        // thread calls fork system call

  //-------------------------------------

  // KERNEL_THREAD LIBRARY TESTS
  // stress tests
   
  // kernel_thread_lib_max_thread_test();      // max threads created by kernel_thread lib
  kernel_thread_lib_multithreading_test();  // multithreaded program written for test
  kernel_thread_uspinlock_test();           // userland spinlock code test
  kernel_thread_semaphore_test();           // synchorization using semaphore

  exit();
}

