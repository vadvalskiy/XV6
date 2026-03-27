#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>

static long long *read_numbers(const char *path, int *out_n){
    FILE *fp = fopen(path, "r");
    if (!fp) { perror("fopen"); return NULL;}
    int cap = 64, n = 0;
    long long *arr = malloc(cap * sizeof *arr);
    if (!arr) {fclose(fp); return NULL;}
    long long v;
    while (fscanf(fp, " %lld", &v) == 1){
        if (n == cap){
            cap *= 2;
            long long *tmp = realloc(arr, cap * sizeof *arr);
            if (!tmp) {free(arr); fclose(fp); return NULL;}
            arr = tmp;}
        arr[n++] = v;
        int c = fgetc(fp);
        if (c != ',' && c != EOF) ungetc(c, fp);
    }

    fclose(fp);
    *out_n = n;
    return arr;
}

static long long reduce(const long long *arr, int n,long long (*f)(long long, long long)){
    long long acc = arr[0];
    for (int i = 1; i < n; i++) acc = f(acc, arr[i]);
    return acc;}

long long sequential_compute(const char *path,long long (*f)(long long, long long)){
    int n;
    long long *arr = read_numbers(path, &n);
    if (!arr || n == 0){
        free(arr);
        fprintf(stderr, "sequential_compute: no numbers read\n");
        return LLONG_MIN;
    }
    long long acc = arr[n - 1];
    for (int i = n - 1; i-- > 0; ) acc = f(arr[i], acc);
    free(arr);
    return acc;
}

long long parallel_compute(const char *path,int n_proc,long long (*f)(long long, long long)){
    if (n_proc <= 0){
        fprintf(stderr, "parallel_compute: n_proc must be > 0\n");
        return LLONG_MIN;}

    int n;
    long long *arr = read_numbers(path, &n);
    if (!arr || n == 0){
        free(arr);
        fprintf(stderr, "parallel_compute: no numbers read\n");
        return LLONG_MIN;}

    if (n_proc > n) n_proc = n;
    int chunk = n / n_proc;
    int leftover = n % n_proc;
    int (*pipes)[2] = malloc(n_proc * sizeof *pipes);
    pid_t *pids = malloc(n_proc * sizeof *pids);
    if (!pipes || !pids) {
        free(arr); free(pipes); free(pids);
        return LLONG_MIN;
    }

    for (int i = 0; i < n_proc; i++){
        if (pipe(pipes[i]) == -1){
            perror("pipe");
            for (int j = 0; j < i; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);}
            free(arr); free(pipes); free(pids);
            return LLONG_MIN;
        }
    }

    int spawned = 0;   
    for (int i = 0; i < n_proc; i++){
        int start = i*chunk;
        int end = start + chunk + (i == n_proc - 1 ? leftover : 0);

        pids[i] = fork();
        if (pids[i] < 0){
            perror("fork");
            for (int j = 0; j < spawned;j++){
                kill(pids[j], SIGTERM);
                waitpid(pids[j], NULL, 0);}
            for (int j = 0; j < n_proc;j++){
                close(pipes[j][0]);
                close(pipes[j][1]);}
            free(arr); free(pipes); free(pids);
            return LLONG_MIN;}

        if (pids[i] == 0) {
            for (int j = 0; j < n_proc; j++) {
                close(pipes[j][0]);              
                if (j != i) close(pipes[j][1]);}
            long long partial = reduce(arr + start, end - start, f);
            int written = write(pipes[i][1], &partial, sizeof partial);
            (void)written;
            close(pipes[i][1]);
            free(arr); free(pipes); free(pids);
            _exit(0);
        }
        spawned++;}

    for (int i = 0; i < n_proc; i++)
        close(pipes[i][1]);

    long long *partials = malloc(n_proc * sizeof *partials);
    if (!partials) {
        for (int i = 0; i < n_proc; i++) close(pipes[i][0]);
        for (int i = 0; i < n_proc; i++) waitpid(pids[i], NULL, 0);
        free(arr); free(pipes); free(pids);
        return LLONG_MIN;
    }

    for (int i = 0; i < n_proc; i++){
        int r = read(pipes[i][0], &partials[i], sizeof partials[i]);
        if (r != (int)sizeof partials[i]){
            fprintf(stderr, "parallel_compute: short read from child %d\n", i);
            partials[i] = 0;
        }
        close(pipes[i][0]);}
    for (int i = 0; i < n_proc; i++) waitpid(pids[i], NULL, 0);

    long long result = reduce(partials,n_proc,f);

    free(arr); free(pipes); free(pids); free(partials);
    return result;
}

#ifdef COMPUTE_TEST
long long add(long long a, long long b) { return a + b;}
long long mul(long long a, long long b) { return a * b;}
long long mx (long long a, long long b) { return a > b? a : b;}

int main(int argc, char *argv[])
{
    const char *path = argc > 1 ? argv[1] : "numbers.txt";

    printf("Part1-sequential_compute\n");
    printf("sum = %lld\n", sequential_compute(path, add));
    printf("product = %lld\n", sequential_compute(path, mul));
    printf("max = %lld\n", sequential_compute(path, mx));

    printf("\nPart2-parallel_compute (4 procs)\n");
    printf("sum  = %lld\n", parallel_compute(path, 4, add));
    printf("prod = %lld\n", parallel_compute(path, 4, mul));
    printf("max  = %lld\n", parallel_compute(path, 4, mx));

    return 0;
}
#endif


