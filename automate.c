#include <stdio.h>
#include <stdlib.h>
#include <time.h>

long long sequential_compute(const char *path,
                             long long (*f)(long long, long long));

long long parallel_compute(const char *path,
                           int n_proc,
                           long long (*f)(long long, long long));

long long add(long long a, long long b){ return a + b; }
long long mul(long long a, long long b){ return a * b; }
long long mx (long long a, long long b){ return a > b ? a : b; }

void generate_numbers(const char *path, int N)
{
    FILE *fp = fopen(path,"w");
    if(!fp)
    {
        perror("fopen");
        exit(1);
    }

    for(int i=0;i<N;i++)
    {
        fprintf(fp,"%d",rand()%100 + 1);
        if(i < N-1) fprintf(fp,",");
    }

    fclose(fp);
}

int main()
{
    const char *path = "numbers.txt";

    srand(time(NULL));

    FILE *out = fopen("results.csv","w");
    if(!out)
    {
        perror("fopen");
        return 1;
    }

    fprintf(out,"N,seq_time,par_time\n");

    for(int N = 1000; N <= 5000000; N += 100000)
    {
        generate_numbers(path,N);

        clock_t start,end;

        // sequential 
        start = clock();
        sequential_compute(path,add);
        sequential_compute(path,mul);
        sequential_compute(path,mx);
        end = clock();

        double seq_time =
        (double)(end-start)/CLOCKS_PER_SEC;

        // parallel
        start = clock();
        parallel_compute(path,7,add);
        parallel_compute(path,7,mul);
        parallel_compute(path,7,mx);
        end = clock();

        double par_time =
        (double)(end-start)/CLOCKS_PER_SEC;

        fprintf(out,"%d,%f,%f\n",N,seq_time,par_time);

        printf("Test N=%d complete\n",N);
    }

    fclose(out);

    printf("Results saved to results.csv\n");

    return 0;
}