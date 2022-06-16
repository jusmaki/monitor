/*
 * (C) Copyright 1996, Jussi Maki
 * Email: Jussi.Maki@csc.fi
 */

#include <sys/types.h>
#include <sys/sysinfo.h>

#if !defined(AIXv3r1) && !defined(AIXv3r2)
void get_cpuinfo(struct cpuinfo **cpu, int *n_cpus);
void print_cpuinfo(double refresh_time, struct cpuinfo **, int n_cpus, int ci);
#else
struct cpuinfo {
  int dummy;
};
#endif
