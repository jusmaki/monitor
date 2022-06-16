#include <sys/types.h>
#ifdef AIX
#include <sys/iostat.h>
#else
struct  dkstat {
  int x;
};
#endif

int get_dkstat(struct dkstat **dkstat_list);
int summary_dkstat(struct dkstat **sum, struct dkstat *dk1, struct dkstat *dk2, int *active);
