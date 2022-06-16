/*
 * get_topcpuP.h  -- get top cpu users private include file
 */
#define swap_ptrs(cur,old,save) save=cur;cur=old;old=save

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define NPROCS 2000

extern int topflag_showusername;

struct procsortinfo {
     int index; /* index to previous procinfo-array */
     double deltacputime;
     double cputime;
     long   deltapageflt;
   };

#ifdef AIX
extern struct procinfo top_proc1[];
extern struct userinfo top_user1[];
extern struct procinfo top_proc2[];
extern struct userinfo top_user2[];
extern struct procsortinfo top_sortinfo[];
#endif

extern int top_getprocinfo();
extern double top_calcsortinfo();
extern void get_topdata(topcpu_t *top, 
		 int ntops, 
		 struct procinfo *proc,
		 struct userinfo *user, 
		 struct procsortinfo *procsortinfo,
		 int nproc, 
		 double cpusum);

