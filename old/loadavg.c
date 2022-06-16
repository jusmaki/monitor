/* loadavg.c
** monitor (AIX 3 System monitor) - loadavg part
**
** This version includes patches for aix3.2.
** Load average values are a bit unstable in aix3.2
** due to changed algorithm which kernel uses to modify
** runque values. (Algorithm is unknown to me; jmaki 24.6.92)
*/

/* update load-values after this time */
#define LOAD_UPDATE_SECS 5

/* ring buffers for runque and runocc values */
#define SIZE_STORE ((60/LOAD_UPDATE_SECS)*15+1)
static int store_runque[SIZE_STORE];
static int store_runocc[SIZE_STORE];
/* ring buffer index values */
static int rbi_current=SIZE_STORE-1;
static int rbi_load1=SIZE_STORE-60/LOAD_UPDATE_SECS-1;
static int rbi_load5=SIZE_STORE-(5*60)/LOAD_UPDATE_SECS-1;
static int rbi_load15=SIZE_STORE-(15*60)/LOAD_UPDATE_SECS-1;


calcloadavg(load1,load5,load15)
double *load1,*load5,*load15;
{
  int runque,runocc;
  int runocc_1,runocc_5,runocc_15;
  runque=store_runque[rbi_current];
  runocc=store_runocc[rbi_current];
  runocc_1 = runocc -  store_runocc[rbi_load1];
  runocc_5 = runocc -  store_runocc[rbi_load5];
  runocc_15 = runocc - store_runocc[rbi_load15];
  *load1 = (double)(runque - store_runque[rbi_load1])
    / (double) (runocc_1);
  *load5 = (double)(runque - store_runque[rbi_load5])
    / (double) (runocc_5);
  *load15 = (double)(runque - store_runque[rbi_load15])
    / (double) (runocc_15);
#ifdef AIXv3r1
  /* decrement loadvalue by kproc's (idle process) value */
  *load1 = *load1 - 1.0;
  *load5 = *load5 - 1.0;
  *load15 = *load15 - 1.0;
#endif
#ifdef DEBUG
  printf("calcloadavg: Drunque Drunocc %d %d\n",runque - store_runque[rbi_load1],runocc_1);
#endif
}

static loadavg_init(runque,runocc)
int runque,runocc;
{
  int i;
  for (i=1;i<SIZE_STORE;i++) {
    store_runque[SIZE_STORE-i] = runque;
    store_runocc[SIZE_STORE-i] = runocc-i*LOAD_UPDATE_SECS;
  }
}

update_loadavg(runque,runocc)
{
  static int prev_update=0;

  if (prev_update==0) {loadavg_init(runque,runocc);prev_update=1;}
  if (time(0)-prev_update > LOAD_UPDATE_SECS) {
    loadavg_put(runque,runocc);
    prev_update=time(0);
  }
}

#define RB_INC(index) ((index+1>=(SIZE_STORE))?(0):(index+1))

loadavg_put(runque,runocc)
int runque,runocc;
{
  int rbi_prev;
  static int prev_runocc=0;
  
  rbi_prev = rbi_current;
  if (prev_runocc == 0) prev_runocc = store_runocc[rbi_prev];
  rbi_current = RB_INC(rbi_current);
  rbi_load1   = RB_INC(rbi_load1);
  rbi_load5   = RB_INC(rbi_load5);
  rbi_load15  = RB_INC(rbi_load15);
#ifdef AIXv3r1
  store_runque[rbi_current]=runque;
  store_runocc[rbi_current]=runocc;
#else
  /* runque calculation has been changed  AIXv3r2:                      */
  /* Idle process is not calculated among running processes anymore     */
  /* and runocc values are not updated every second (only when needed). */
  /* runque and runocc values are updated every 0.5 ... 60 seconds      */
  /* depending on system load and other situations. */
  /* this code tries to make new runocc and runque values look like old ones  */
  /* If anybody in NET can give me a nicer way to do it, please do it */
  store_runque[rbi_current]=runque;
  if (runocc - prev_runocc < LOAD_UPDATE_SECS)
    store_runocc[rbi_current] = store_runocc[rbi_prev]
      + LOAD_UPDATE_SECS + runocc-prev_runocc;
  else
    store_runocc[rbi_current] = store_runocc[rbi_prev] + runocc-prev_runocc;
  prev_runocc = runocc;
#endif

}
