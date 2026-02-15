This data comes from test tests.c file

Setup
Number of children: 3
Tickets per child:
Child 1: 10 tickets
Child 2: 20 tickets
Child 3: 30 tickets
Workload
Each child process ran a CPU-bound loop, performing continuous computations for the duration of the test.
Observed Relative Shares
Child	Tickets	CPU Ticks Used	Relative Share (%)
1	      10	     597             31.3%
2	      20	     648	         34.0%
3	      30         663             34.7%
The observed CPU usage roughly reflects the ticket proportions, though variance exists.

Notes
Small deviations from the theoretical ratios (1:2:3) occur due to the random lottery selection at each scheduler tick.

Over longer runs, these percentages converge toward the expected ratios as random fluctuations average out.