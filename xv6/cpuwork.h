#ifndef XV6_CPUWORK_H
#define XV6_CPUWORK_H

// Deterministic CPU work. The amount of work depends on iteration count rather
// than wall-clock time, and no system calls occur in the inner loop.
static void
cpu_work(int units, int intensity)
{
  volatile uint state;
  int unit;
  int i;
  int iterations;

  if(units < 0)
    units = 0;
  if(intensity < 1)
    intensity = 1;
  iterations = 20000 * intensity;
  state = (uint)getpid() ^ 0x9e3779b9U;
  for(unit = 0; unit < units; unit++)
    for(i = 0; i < iterations; i++)
      state = state * 1664525U + 1013904223U + (uint)i;

  // Preserve the volatile computation without adding normal output noise.
  if(state == 0x7fffffffU)
    printf(1, "cpu_work sentinel: %d\n", (int)state);
}

#endif
