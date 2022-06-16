/* monitor -- AIX RS/6000 System monitor 
 *
 * Copyright (c) 1991, 1992, 1993, 1994 Jussi Maki. All Rights Reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE THIS PROGRAM 
 * AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL COPYRIGHTS.
 * Email: jmaki@hut.fi
 */
#include <sys/types.h>

#define TOPCPU_USERNAME_LEN 9
#define TOPCPU_PROGNAME_LEN 32

typedef struct {                                /*        Size */
  uid_t   uid;                                  /*           4 */
  pid_t   pid;                                  /*           4 */
  u_char  pri;                                  /*           1 */
  u_char  nice;                                 /*           1 */
  u_char  stat;                                 /*           1 */
  char    username[TOPCPU_USERNAME_LEN];        /*           9 */
  char    progname[TOPCPU_PROGNAME_LEN];        /*          20 */
  u_long  memsize_1k;                           /*           4 */
  u_long  ressize_1k;                           /*           4 */
  u_long  pageflt;                              /*           4 */
  time_t  starttime;                            /*           4 */
  time_t  cpu_utime;                            /*           4 */
  time_t  cpu_stime;                            /*           4 */
  u_short   deltapageflt;                       /*           2 */
  u_short  cputime_prs;     /* divide by 10  */ /* CPU%      2 */
} topcpu_t;                                    

int get_topcpu(topcpu_t *top, int ntops);
/* Usage:
   topcpu_t topcpu[100];
   int n;
   n = get_topcpu(topcpu,100);
 */

