/*
 * (C) Copyright 1996, Jussi Maki
 * Email: Jussi.Maki@csc.fi
 */

#if !defined(AIXv3r1) && !defined(AIXv3r2)

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/systemcfg.h>
#include <sys/sysconfig.h>
#include "get_cpuinfo.h"
#include "getkmemdata.h"


#ifdef MAIN
main(int argc, char **argv)
{
  int i,ci=0;
  struct cpuinfo *cpus[2]={NULL,NULL};
  int n_cpus;
  get_cpuinfo(&cpus[ci], &n_cpus);ci^=1;
  initscr();clear();move(0,0);
  printw("RS/6000 SMP System monitor, (c) Jussi Maki, 1996\n");
  printw("n_cpus: %d\n", n_cpus);

  while (1) {
    sleep(1);
    get_cpuinfo(&cpus[ci], &n_cpus);ci^=1;
    print_cpuinfo(1.0, cpus, n_cpus, ci);
    refresh();
  }
}
#endif /* MAIN */

void print_cpuinfo(double rs, struct cpuinfo **cpus, int n_cpus, int ci)
{
  int i;
  int cp;
  double c_idle,c_user,c_kern,c_wait,c_sum;

  struct cpuinfo cpusum;
  cp = ci^1;
  mon_print(5,0,"CPU USER KERN WAIT IDLE%% PSW SYSCALL WRITE  READ WRITEkb  READkb\n");  
  memset(&cpusum, 0, sizeof(cpusum));
  for (i=0; i<n_cpus; i++) {
    cpusum.cpu[CPU_USER] += c_user = cpus[cp][i].cpu[CPU_USER]-cpus[ci][i].cpu[CPU_USER];
    cpusum.cpu[CPU_KERNEL] += c_kern = cpus[cp][i].cpu[CPU_KERNEL]-cpus[ci][i].cpu[CPU_KERNEL];
    cpusum.cpu[CPU_WAIT] += c_wait = cpus[cp][i].cpu[CPU_WAIT]-cpus[ci][i].cpu[CPU_WAIT];
    cpusum.cpu[CPU_IDLE] += c_idle = cpus[cp][i].cpu[CPU_IDLE]-cpus[ci][i].cpu[CPU_IDLE];
    cpusum.pswitch  += cpus[cp][i].pswitch-cpus[ci][i].pswitch;
    cpusum.syscall  += cpus[cp][i].syscall-cpus[ci][i].syscall;
    cpusum.sysread  += cpus[cp][i].sysread-cpus[ci][i].sysread;
    cpusum.syswrite += cpus[cp][i].syswrite-cpus[ci][i].syswrite;
    cpusum.writech  += cpus[cp][i].writech-cpus[ci][i].writech;
    cpusum.readch   += cpus[cp][i].readch-cpus[ci][i].readch;
    c_sum = c_idle+c_user+c_kern+c_wait;
    c_idle = 100.0*c_idle/c_sum;  c_user = 100.0*c_user/c_sum;
    c_kern = 100.0*c_kern/c_sum;  c_wait = 100.0*c_wait/c_sum;
    mon_print(6+i,0,"#%-2d %4.0f %4.0f %4.0f %4.0f %4.0f %7.0f %5.0f %5.0f %7.2f %7.2f\n", 
	   i,
	   c_user,c_kern,c_wait,c_idle, 
	   (cpus[cp][i].pswitch-cpus[ci][i].pswitch)/rs,
	   (cpus[cp][i].syscall-cpus[ci][i].syscall)/rs,
	   (cpus[cp][i].syswrite-cpus[ci][i].syswrite)/rs,
	   (cpus[cp][i].sysread-cpus[ci][i].sysread)/rs,
	   (cpus[cp][i].writech-cpus[ci][i].writech)/1024.0/rs,
	   (cpus[cp][i].readch-cpus[ci][i].readch)/1024.0/rs);
  }
  mon_print(6+i,0,"=====================================================================\n");
  c_idle = cpusum.cpu[CPU_IDLE];
  c_user = cpusum.cpu[CPU_USER];
  c_kern = cpusum.cpu[CPU_KERNEL];
  c_wait = cpusum.cpu[CPU_WAIT];
  c_sum = c_idle+c_user+c_kern+c_wait;
  c_idle = 100.0*c_idle/c_sum;  c_user = 100.0*c_user/c_sum;
  c_kern = 100.0*c_kern/c_sum;  c_wait = 100.0*c_wait/c_sum;
  
  mon_print(6+i+1,0,"SUM %4.0f %4.0f %4.0f %4.0f %4.0f %7.0f %5.0f %5.0f %7.2f %7.2f\n",
	 c_user,c_kern,c_wait,c_idle,
	 cpusum.pswitch/rs, cpusum.syscall/rs,
	 cpusum.syswrite/rs, cpusum.sysread/rs,
	 cpusum.writech/1024.0/rs, cpusum.readch/1024.0/rs);
}


#include <nlist.h>
#include "get_sysvminfo.h"
static struct nlist kernelnames[] = {
    {"cpuinfo", 0, 0, 0, 0, 0},
    {NULL, 0, 0, 0, 0, 0},
    };
#define N_VALUE(index) (kernelnames[index].n_value)
#define NUMBER_OF_KNSTRUCTS 1
#define NLIST_CPUINFO 0

void get_cpuinfo(struct cpuinfo **cpu, int *n_cpus)
{
  static int initted = 0;
  static struct cpuinfo *cpuinfo_buf;
  
  *n_cpus = _system_configuration.ncpus;
  if (! initted) {
    initted = 1;
    if (knlist(kernelnames,NUMBER_OF_KNSTRUCTS,sizeof(struct nlist)) == -1){
	perror("knlist, entry not found");
    }
  }
  if (! *cpu) *cpu=(struct cpuinfo *)calloc(sizeof(struct cpuinfo), (*n_cpus));
  getkmemdata((char *)*cpu, sizeof(struct cpuinfo)* (*n_cpus), 
	      (caddr_t)N_VALUE(NLIST_CPUINFO));
}
#else
void print_cpuinfo(double rs, struct cpuinfo **cpus, int n_cpus, int ci)
{
}

void get_cpuinfo(struct cpuinfo **cpu, int *n_cpus)
{
}
#endif /* ! AIX3.1 or AIX3.2 */
