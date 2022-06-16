/* Fast ps like utility "fps" -- Jussi Maki, Helsinki University of */
/* Technology, Computing Centre, jmaki@hut.fi, February 21, 1992 */
/* compile this program in AIX3 with eg. command:
 *   cc -o fps fps.c
 */

#include <stdio.h>
#include <procinfo.h>
#include <sys/time.h>

#define NPROCS 10000

/* system call to get process table */
extern getproc(struct procinfo *procinfo, int nproc, int sizproc);
/* struct  procinfo *procinfo;   pointer to array of procinfo struct    */
/* int  nproc;                   number of user procinfo struct         */
/* int  sizproc;                 size of expected procinfo structure    */

/* system call to get program arguments */
extern getargs(struct procinfo *procinfo, int plen, char *args, int alen);
/* struct  procinfo *procinfo;   pointer to array of procinfo struct    */
/* int  plen;                    size of expected procinfo struct       */
/* char *args;                   pointer to user array for arguments    */
/* int  alen;                    size of expected argument array        */

/* system call to get user-area variables according to process */
extern getuser(struct procinfo *procinfo, int plen, void *user, int ulen);
/* struct  procinfo *procinfo;   ptr to array of procinfo struct
 * int     plen;                 size of expected procinfo struct
 * void   *user;                 ptr to array of userinfo struct, OR user
 * int     ulen;                 size of expected userinfo struct
 */

struct procsortinfo {
     int index; /* index to previous procinfo-array */
     double deltacputime;
     double cputime;
   };

/* memory allocation doesn't really happen until
 * memory is touched so these large arrays don't 
 * waste memory in AIX 3
 */
struct procinfo proc1[NPROCS];
struct userinfo user1[NPROCS];

/* array of chars which contains process statussymbols */
char statch[] = {'?','S','?','R','T','Z','S'};

#define SORTBYCPUTIME 1
#define SORTBYCPUPERCENT 2

FILE *logfile;

int show_allprocs=0,show_mem;

int proc_to_watch=-1;
int sleep_time=1;
int continous=0;

main(argc,argv)
int argc;
char **argv;
{
  int result;
  int i;
  int nproc1;
  double cpusum;

  while (argc>=2) {
    if (argv[1][0] != '-') usage();
    switch(argv[1][1]) {
      case 'a': show_allprocs=1; break;
      case 'm': show_mem=1; break;
      case 'p': proc_to_watch=atoi(argv[2]); argv++;argc--; break;
      case 'c': continous=1; break;
      case 's': sleep_time=atoi(argv[2]); argv++; argc--; break;
      default: usage();
    }
    argc--;
    argv++;
  }

  do {
    nproc1 = getprocessinfo(proc1,user1);
    if (proc_to_watch >= 0) 
      print_one_proc(proc_to_watch,proc1,user1,nproc1);
    else
      printprocs(proc1,user1,nproc1);
    if (continous) sleep(sleep_time);
  } while (continous);
}

usage()
{
  fprintf(stderr,"Usage: fps [-m] [-a] [-p pid] [-s sleep] [-continous]\n");
  fprintf(stderr,"\tfps is a fast ps which shows processes without interpreting uids\n");
  exit(1);
}

double walltime()
{
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv,&tz);
  return(tv.tv_sec + tv.tv_usec*1.0e-6);
}

print_one_proc(pid_t pid, struct procinfo *proc, struct userinfo *user, int nproc) 
{
  static int first_call=1;
  static double old_walltime,new_walltime;
  static double old_cputime,new_cputime;
  static unsigned long old_pagefault, new_pagefault;
  int i;

  for (i=0;i<nproc; i++) if (proc[i].pi_pid == pid) break;
  if (i>=nproc) {fprintf(stderr, "No such process"); exit(0); }
  old_walltime = new_walltime;  old_cputime  = new_cputime;
  old_pagefault = new_pagefault;
  new_walltime = walltime();
  new_cputime  = user[i].ui_ru.ru_utime.tv_sec + user[i].ui_ru.ru_utime.tv_usec*1.0e-6 
      + user[i].ui_ru.ru_stime.tv_sec + user[i].ui_ru.ru_stime.tv_usec*1.0e-6;
  new_pagefault = proc[i].pi_majflt;
  if (first_call) {
    printf("#WATCH PID %d\n", pid); first_call=0;
    printf("TOTCPU     CPU%%  RESMEM  VIRMEM PGFLTS WAITCHAN WAITTYPE\n");
  } else {
    printf("%-8.1f %5.2f%% %6.2fM %6.2fM %6d %8x %8x\n",
	   new_cputime,
	   (new_cputime-old_cputime)/(new_walltime - old_walltime)*100.0,
	   (user[i].ui_trss*4+user[i].ui_drss*4)/1024.0,
	   (user[i].ui_tsize)/1024.0/1024.0+user[i].ui_dvm*4/1024.0,
	   new_pagefault - old_pagefault,
	   proc[i].pi_wchan, 
	   proc[i].pi_wtype);
  }
}

/*
 * CPU -field shows the latest CPU-time usage accumalated in AIX scheduler
 * PRI -field is the current priority used in AIX scheduler
 * USER- and SYSTIME show process cputime by usertime and systemtime
 */

printprocs(proc,user,nproc)
struct procinfo *proc;
struct userinfo *user;
int nproc;
{
  int i;
  if (show_mem) 
    printf("   PID   PPID PGRPID   UID  SIZE  RES STAT  USER-  SYSTIME CHILDS COMMAND\n");
  else
    printf("   PID   PPID PGRPID   UID PRI NICE CPU STAT USER-  SYSTIME CHILDS COMMAND\n");
  for (i=0; i<nproc; i++) {
      if (proc[i].pi_cpu != 0 || show_allprocs) {
	printf("%6d %6d %6d %5d ",
	     proc[i].pi_pid,
	     proc[i].pi_ppid,proc[i].pi_pgrp,
	     proc[i].pi_uid);
	if (show_mem) 
	  printf("%4.1fM %4.1fM ",
		 (user[i].ui_tsize)/1024.0/1024.0+user[i].ui_dvm*4/1024.0,
		 (user[i].ui_trss*4+user[i].ui_drss*4)/1024.0);
		 
	else 
	  printf("%3d %4d %3d ",
	     proc[i].pi_pri,
	     proc[i].pi_nice-20,
	     proc[i].pi_cpu);
	if (proc[i].pi_stat != SZOMB) {
	  printf("%c %8.2f %8.2f %6.2f %s",
	     statch[proc[i].pi_stat],
	     user[i].ui_ru.ru_utime.tv_sec+user[i].ui_ru.ru_utime.tv_usec*1e-6,
	     user[i].ui_ru.ru_stime.tv_sec+user[i].ui_ru.ru_stime.tv_usec*1e-6,
	     user[i].ui_cru.ru_utime.tv_sec+user[i].ui_cru.ru_utime.tv_usec*1e-6
   	     +user[i].ui_cru.ru_stime.tv_sec+user[i].ui_cru.ru_stime.tv_usec*1e-6,
	     user[i].ui_comm);
	  if (proc[i].pi_flag&SKPROC) printf(" (kproc)");
	  printf("\n");
	} else {
	  printf("%6d %6d ",proc[i].pi_utime,proc[i].pi_stime);
	  printf("(ZOMBIE)\n");
	}
      }
  }
}


int getprocessinfo(procinfo,userinfo)
struct procinfo *procinfo;
struct userinfo *userinfo;
{
  int nproc;
  char *swappername = "swapper";
  int i;

  /* get the whole process table */
  nproc = getproc(procinfo,NPROCS,sizeof(struct procinfo));
  for (i=0; i<nproc; i++) { /* get each user-area entrys by process */
    getuser(&procinfo[i],sizeof(struct procinfo),
	    &userinfo[i],sizeof(struct userinfo));
  }
  strcpy(userinfo[0].ui_comm,swappername); /* first process is always pid 0 */
  return nproc;
}
