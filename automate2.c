#include <stdio.h>
#include <time.h>

long long sequential_compute(const char *path,
                             long long (*f)(long long, long long));

long long parallel_compute(const char *path,
                           int n_proc,
                           long long (*f)(long long, long long));

long long add(long long a, long long b){ return a + b; }
long long mul(long long a, long long b){ return a * b; }
long long mx (long long a, long long b){ return a > b ? a : b; }

int main()
{
    const char *path = "numbers.txt";

    FILE *out = fopen("process_scaling.csv","w");
    if(!out)
    {
        perror("fopen");
        return 1;
    }

    fprintf(out,"processes,time\n");

    // different numbers of processes
    for(int p = 1; p <= 16; p++)
    {
        clock_t start,end;

        start = clock();

        parallel_compute(path,p,add);
        parallel_compute(path,p,mul);
        parallel_compute(path,p,mx);

        end = clock();

        double time_taken =
        (double)(end-start)/CLOCKS_PER_SEC;

        fprintf(out,"%d,%f\n",p,time_taken);

        printf("Processes %d done\n",p);
    }

    fclose(out);

    printf("Results saved to process_scaling.csv\n");

    return 0;
}