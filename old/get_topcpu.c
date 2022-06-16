/* monitor -- AIX RS/6000 System monitor 
 *
 * Copyright (c) 1991, 1992, 1993, 1994, 1995 Jussi Maki. All Rights Reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE THIS PROGRAM 
 * AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL COPYRIGHTS.
 * Email: jmaki@csc.fi
 * URL: http://www.csc.fi/~jmaki
 */

#ifdef AIX
#include <procinfo.h>
#endif
#include "get_topcpu.h"
#include "get_topcpuP.h"
#include "getproc.h"

int nproc1=0,nproc2=0;
#ifdef AIX
struct procinfo proc1[NPROCS];
struct procinfo proc2[NPROCS];
struct userinfo user1[NPROCS];
struct userinfo user2[NPROCS];
struct procsortinfo top_sortinfo[NPROCS];
#endif
int *nproc_old,*nproc_cur,*nproc_save;
struct procinfo *proc_old, *proc_cur,*proc_save;
struct userinfo *user_old, *user_cur,*user_save;

int get_topcpu(topcpu_t *top, int ntops)
{
  double cpusum;
  static int initted=0;
#ifdef AIX
  if (!initted) {
    proc_old  = proc1;    user_old  = user1;    nproc_old = &nproc1;
    proc_cur  = proc2;    user_cur  = user2;    nproc_cur = &nproc2;
    initted=1;
  }
  ntops = min(ntops, NPROCS);
  *nproc_cur = top_getprocinfo(proc_cur, user_cur);
  cpusum = top_calcsortinfo(proc_old, user_old, *nproc_old, 
			    proc_cur, user_cur, *nproc_cur, top_sortinfo);
  get_topdata(top, ntops, proc_cur, user_cur, top_sortinfo, 
	      *nproc_cur, cpusum);
  swap_ptrs(proc_cur,proc_old,proc_save);
  swap_ptrs(nproc_cur,nproc_old,nproc_save);
  swap_ptrs(user_cur,user_old,user_save);
  return(min(*nproc_cur,ntops));
#else
  return 0;
#endif
}

void get_topdata(topcpu_t *top, int ntops, struct procinfo *proc,
		 struct userinfo *user, struct procsortinfo *procsortinfo,
		 int nproc, double cpusum)
{
  int i,j;
  for (j=0; j<min(ntops,nproc); j++) {
    i = procsortinfo[j].index;
    if (proc[i].pi_flag & SKPROC) {
      char buf[256];
      strcpy(buf, "Kernel (");
      strcat(buf, user[i].ui_comm);
      strcat(buf, ")");
      strncpy(top[j].progname, buf, TOPCPU_PROGNAME_LEN-1);
    } else {
      strncpy(top[j].progname, user[i].ui_comm, TOPCPU_PROGNAME_LEN-1);
    }
    top[j].progname[TOPCPU_PROGNAME_LEN-1]='\0';
    top[j].pid         = proc[i].pi_pid;
    top[j].uid         = proc[i].pi_uid;
    top[j].pri         = proc[i].pi_pri;
    top[j].nice        = proc[i].pi_nice;
    top[j].stat        = proc[i].pi_stat; /* tsize is text size in bytes */
    top[j].memsize_1k  = user[i].ui_tsize/1024+user[i].ui_dvm*4;
    top[j].ressize_1k  = (user[i].ui_drss+user[i].ui_trss)*4;
    top[j].pageflt     = user[i].ui_ru.ru_minflt;
    top[j].starttime   = user[i].ui_start;
    top[j].cpu_utime   = user[i].ui_ru.ru_utime.tv_sec;
    top[j].cpu_stime   = user[i].ui_ru.ru_stime.tv_sec;
    top[j].deltapageflt= procsortinfo[j].deltapageflt;
    top[j].cputime_prs = ((double)procsortinfo[j].deltacputime
			  /(double)cpusum * 1000.0);
  }
}


/* sort-order by deltacputime, second key is cputime */
int cmp_deltacputime(struct procsortinfo *a, struct procsortinfo *b)
{
  if (a->deltacputime > b->deltacputime) return (-1);
  else if (a->deltacputime < b->deltacputime) return ( 1);
  else if (a->cputime > b->cputime) return (-1);
  else if (a->cputime < b->cputime) return ( 1);
  else return(0);
}

/* lets assume that procinfo-array is kept in order by PID */
double top_calcsortinfo(proc1,user1,nproc1,proc2,user2,nproc2,procsortinfo)
struct procinfo *proc1; /* older values */
struct userinfo *user1;
int nproc1;
struct procinfo *proc2; /* current values */
struct userinfo *user2;
int nproc2;
struct procsortinfo *procsortinfo;
{
    int i,j;
    double cpusum=0.0;
    int procmatch;
    struct procinfo *p1,*p2;
    struct userinfo *u1,*u2;
  
    /* lets try finding same process to calculate time process got from cpu */
    p1 = proc1; u1 = user1;
    p2 = proc2; u2 = user2;
    p1[nproc1].pi_pid = 0x7ffffff; /* lets put largest positive long to stop 
				   ** while loop not to go over nproc1 items, 
				   ** and lets hope there are not pid with 
				   ** that number */
    p2[nproc2].pi_pid = 0;         /* and something smaller here */
    for (i=0; i<nproc2; i++) { 
	while (p1->pi_pid < p2->pi_pid) {p1++;u1++;}
	procmatch = (p1->pi_pid == p2->pi_pid);
	
	/* if process is zombie we don't trust the information */
	if (p2->pi_stat == SZOMB) { 
	    procsortinfo[i].cputime = procsortinfo[i].deltacputime = 0.0;
	} else {
	    procsortinfo[i].cputime =
	      u2->ui_ru.ru_utime.tv_sec
		+ u2->ui_ru.ru_utime.tv_usec*1.0e-6
		  + u2->ui_ru.ru_stime.tv_sec
		    + u2->ui_ru.ru_stime.tv_usec*1.0e-6;
	    procsortinfo[i].deltacputime = procsortinfo[i].cputime;
	    procsortinfo[i].deltapageflt = u2->ui_ru.ru_minflt;
	    if (procmatch) { 
		/* lets watch out for processes which have identical pid 
		 * but which are not same process between two samples  */
		if (u1->ui_start == u2->ui_start) {
		  procsortinfo[i].deltacputime -=  
		    (u1->ui_ru.ru_utime.tv_sec
		     + u1->ui_ru.ru_utime.tv_usec*1.0e-6
		     + u1->ui_ru.ru_stime.tv_sec
		     + u1->ui_ru.ru_stime.tv_usec*1.0e-6);
		  procsortinfo[i].deltapageflt -= u1->ui_ru.ru_minflt;
		}
	    }
	}
	procsortinfo[i].index = i;
	cpusum += procsortinfo[i].deltacputime;
	p2++;u2++;
    }
    qsort(procsortinfo,nproc2,sizeof(struct procsortinfo),cmp_deltacputime);
    return(cpusum);
}

/******************************************************************************
 * if a process entry is invalid then fake that this process is a zombie,
 * so it won't be displayd..., jmaki 921019
 */
int top_getprocinfo(struct procinfo *procinfo,struct userinfo *userinfo)
{
  int nproc;
  char *swappername = "swapper";
  int i;
  int st;

  nproc = getproc(procinfo,NPROCS,sizeof(struct procinfo));
  for (i=0; i<nproc; i++) {
    st = getuser(&procinfo[i],sizeof(struct procinfo),
	    &userinfo[i],sizeof(struct userinfo));
    if (st==-1) procinfo[i].pi_stat = SZOMB; 
  }
  strcpy(userinfo[0].ui_comm,swappername); /* first process is always pid 0 */
  return(nproc);
}

