/* Copyright (c) 1991 - 1996, 2011- Jussi Maki. All Rights Reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO RE-DISTRIBUTE 
 * THIS PROGRAM AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL COPYRIGHTS.
 * Email: jusmaki@gmail.com
 */


#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "get_linux.h"


int no_nfs = 0;
int get_sysvminfo(struct sysvminfo **ip)
{
  char line[256];
  if (*ip == NULL) {
    *ip = calloc(1, sizeof(struct sysvminfo));
  }
  FILE *f = fopen("/proc/stat", "r");
  if (f==NULL) {
    fprintf(stderr, "FATAL ERROR: Cannot open /proc/stat");
    return 0;
  }
  //         cpu user nic sys  idle wait irq  sirq
  char cpuid[16];
  unsigned long long cpu_user, cpu_nice, cpu_syst, cpu_idle, cpu_iowait, cpu_irq, cpu_sirq;
  fscanf(f, "%s %llu %llu %llu %llu %llu %llu %llu", &cpuid, &cpu_user, &cpu_nice, &cpu_syst, &cpu_idle, &cpu_iowait, &cpu_irq, &cpu_sirq);
  fclose(f);

  (*ip)->sysinfo.cpu[CPU_USER] = cpu_user;
  (*ip)->sysinfo.cpu[CPU_NICE] = cpu_nice;
  (*ip)->sysinfo.cpu[CPU_KERNEL] = cpu_syst;
  (*ip)->sysinfo.cpu[CPU_IDLE] = cpu_idle;
  (*ip)->sysinfo.cpu[CPU_WAIT] = cpu_iowait;
  (*ip)->sysinfo.cpu[CPU_IRQ] = cpu_irq;
  (*ip)->sysinfo.cpu[CPU_SIRQ] = cpu_sirq;
  struct vmker *vmk = &((*ip)->vmker);
  f = fopen("/proc/meminfo", "r");
  if (f==NULL) {
    fprintf(stderr, "FATAL ERROR: Cannot open /proc/meminfo");
    return 0;
  }
  while (fgets(line, sizeof(line)-1, f)) {
    char key[64];
    unsigned int value;
    sscanf(line, "%s %u", key, &value);
    if (strstr(line, "MemTotal:")) {
      vmk->memtotal = value;
    } else if (strstr(line, "MemFree:")) {
      vmk->memfree = value;
    } else if (strstr(line, "Buffers:")) {
      vmk->buffers = value;
    } else if (strstr(line, "Cached:")) {
      vmk->cached = value;
    } else if (strstr(line, "SwapTotal:")) {
      vmk->swaptotal = value;
    } else if (strstr(line, "SwapFree:")) {
      vmk->swapfree = value;
    }
  }
  fclose(f);

  f = fopen("/proc/stat", "r");
  if (f==NULL) {
    fprintf(stderr, "FATAL ERROR: Cannot open /proc/stat");
    return 0;
  }
  while (fgets(line, sizeof(line)-1, f)) {
    char key[64];
    unsigned long long  value;
    sscanf(line, "%s %llu", key, &value);
    if (strcmp(key, "ctxt")==0) {
      (*ip)->sysinfo.ctxt = value;
    } else if (strcmp(key, "processes")==0) {
      (*ip)->sysinfo.processes = value;
    } else if (strcmp(key, "intr")==0) {
      (*ip)->sysinfo.intr = value;
    }
  }
  fclose(f);

  f = fopen("/proc/vmstat", "r");
  if (f==NULL) {
    fprintf(stderr, "FATAL ERROR: Cannot open /proc/vmstat");
    return 0;
  }
  while (fgets(line, sizeof(line)-1, f)) {
    char key[64];
    unsigned long long  value;
    sscanf(line, "%s %llu", key, &value);
    if (strcmp(key, "pgpgin")==0) {
      (*ip)->vminfo.pageins = value;
    } else if (strcmp(key, "pgpgout")==0) {
      (*ip)->vminfo.pageouts = value;
    } else if (strcmp(key, "pswpin")==0) {
      (*ip)->vminfo.pswpin = value;
    } else if (strcmp(key, "pswpout")==0) {
      (*ip)->vminfo.pswpout = value;
    } else if (strcmp(key, "pgfault")==0) {
      (*ip)->vminfo.pgfault = value;
    }
  }
  fclose(f);
  return 0;
}

#define NUMDISKS 32
int get_dkstat(struct dkstat **ip)
{
  char line[256];
  int numdisks = 0;
  static unsigned int readblocks = 0;
  if (*ip == NULL) {
    *ip = calloc(NUMDISKS, sizeof(struct dkstat));
  }
  struct dkstat *dk = *ip;
  FILE *f = fopen("/proc/diskstats", "r");
  if (f==NULL) {
    fprintf(stderr, "FATAL ERROR: Cannot open /proc/diskstats");
    return 0;
  }
  //         cpu user nic sys  idle wait irq  sirq
  while (1) {
    char diskid[16];
    int major, minor;
    unsigned long long read_ios, read_merges, read_sectors, read_time_tick;
    unsigned long long write_ios, write_merges, write_sectors, write_time_tick;
    int io_pending;
    unsigned long long io_time_tick, io_time_queue;

    if (fgets(line, sizeof(line), f) <= 0) break;
    int numarg;
    numarg = sscanf(line, "%d %d %s %llu %llu %llu %llu %llu %llu %llu %llu %d %llu %llu" , &major, &minor, &diskid, &read_ios, &read_merges, &read_sectors, &read_time_tick, &write_ios, &write_merges, &write_sectors, &write_time_tick, &io_pending, &io_time_tick, &io_time_queue);
    if (strncmp(diskid, "sd", 2)==0 || strncmp(diskid, "hd", 2)==0 || strncmp(diskid, "cciss", 5)==0) {
      // make statistics from the full disks only not per partition
      if (strncmp(diskid, "cciss", 5)==0 && index(diskid, 'p')>0) continue; // skip cciss partitions
      if (strncmp(diskid, "cciss", 5)!=0 && isdigit(diskid[strlen(diskid)-1])) continue; // skip normal partitions
      strcpy(dk[numdisks].diskname, diskid);
      dk[numdisks].dk_rblks = read_sectors;
      dk[numdisks].dk_wblks = write_sectors;
      dk[numdisks].dk_bsize = 512;
      dk[numdisks].dk_xfers = read_ios+read_merges+write_ios+write_merges;
      dk[numdisks].dk_time = io_time_tick/10;
      dk[numdisks].dknextp = &dk[numdisks+1];
      numdisks++;
    }
  }
  fclose(f);
  dk[numdisks-1].dknextp = NULL;
  return numdisks;
}
#define MAXIF 64
int get_ifnet(struct ifnet **ifp)
{
  char line[512];
  if (*ifp == NULL) {
    *ifp = calloc(MAXIF, sizeof(struct ifnet));
  }
  struct ifnet *ifnet = *ifp;
  FILE *f = fopen("/proc/net/dev", "r");
  if (f==NULL) {
    fprintf(stderr, "FATAL ERROR: Cannot open /proc/net/dev");
    return 0;
  }
  while (fgets(line, sizeof(line)-1, f) != NULL) {
    char *p=NULL;
    if ((p=strchr(line, ':'))) {
      char ifname[32];
      unsigned long long recv_bytes;
      unsigned long long recv_pkts;
      unsigned int recv_errs, recv_drop, recv_fifo, recv_frame, recv_comp, recv_mcast;
      unsigned long long send_bytes;
      unsigned long long send_pkts;
      unsigned int send_errs;

      
      *p = ' ';//ifname bytespackets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
      sscanf(line, "%s %lld %lld %d %d %d %d %d %d %lld %lld %d", ifname, 
	     &recv_bytes, &recv_pkts, &recv_errs, &recv_drop, &recv_fifo, &recv_frame, &recv_comp, &recv_mcast,
	     &send_bytes, &send_pkts, &send_errs);
      strcpy(ifnet->if_name, ifname);
      ifnet->if_unit = -1;
      ifnet->if_ibytes = recv_bytes;
      ifnet->if_obytes = send_bytes;
      ifnet->if_ipackets = recv_pkts;
      ifnet->if_opackets = send_pkts;
      ifnet->if_next = ifnet+1;
      ifnet++;
    }
  }
  fclose(f);
  return 0;
}

int get_nfsstat(nfsstat_t **ip)
{
  char line[4096];
  if (*ip == NULL) {
    *ip = calloc(1, sizeof(nfsstat_t));
  }
  FILE *f = fopen("/proc/net/rpc/nfsd", "r");
#define NS3(k) ((*ip)->sv.k)
  if (f!=NULL) {
    while (fgets(line, sizeof(line)-1, f) != NULL) {
      char key[32];
      uint u1;
      if (strstr(line, "rpc ")) {
	sscanf(line, "%s %u %u", key, &((*ip)->rs.calls), &((*ip)->rs.nullrecv));
	NS3(calls) = (*ip)->rs.calls;
      } else if(strstr(line, "proc3 ")) {
	//            1  2  3  4  5  6  7  8  1  2  3  4  5  6  1  2  3  4  5  6  1  2  3  4  
	sscanf(line, "%s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
	       key, &u1, &NS3(null), &NS3(getattr), &NS3(setattr), &NS3(lookup), &NS3(access), &NS3(readlink), //8
               &NS3(read), &NS3(write), &NS3(create), &NS3(mkdir), &NS3(symlink), &NS3(mknod),                 //6
               &NS3(remove), &NS3(rmdir), &NS3(rename), &NS3(link), &NS3(readdir), &NS3(readdirplus),          //6
               &NS3(fsstat), &NS3(fsinfo), &NS3(pathconf), &NS3(commit));                                      //4
      }
    }
    fclose(f);
  }

  f = fopen("/proc/net/rpc/nfs", "r");
  if (f!=NULL) {
#define NC3(k) ((*ip)->cl.k)
    while (fgets(line, sizeof(line)-1, f) != NULL) {
      char key[32];
      uint u1;
      if (strstr(line, "rpc ")) {
	sscanf(line, "%s %u %u", key, &((*ip)->rc.calls), &((*ip)->rc.retrans));
	NC3(calls) = (*ip)->rc.calls;
      } else if(strstr(line, "proc3 ")) {
	//            1  2  3  4  5  6  7  8  1  2  3  4  5  6  1  2  3  4  5  6  1  2  3  4  
	sscanf(line, "%s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
	       key, &u1, &NC3(null), &NC3(getattr), &NC3(setattr), &NC3(lookup), &NC3(access), &NC3(readlink), //8
               &NC3(read), &NC3(write), &NC3(create), &NC3(mkdir), &NC3(symlink), &NC3(mknod),                 //6
               &NC3(remove), &NC3(rmdir), &NC3(rename), &NC3(link), &NC3(readdir), &NC3(readdirplus),          //6
               &NC3(fsstat), &NC3(fsinfo), &NC3(pathconf), &NC3(commit));                                      //4
      }
    }
    fclose(f);
  }


  return 0;
}
int get_cpuinfo(struct cpuinfo **ip, int *ncpus)
{
  if (*ip == NULL) {
    *ip = calloc(1, sizeof(struct cpuinfo));
  }
  *ncpus = 1;
  return 0;
}

int cmp_top(void *a, void *b)
{
  topcpu_t *t1 = (topcpu_t *)a;
  topcpu_t *t2 = (topcpu_t *)b;
  if (t1->cputime_prs > t2->cputime_prs) return -1;
  else if (t1->cputime_prs < t2->cputime_prs) return 1;
  else {
    if (t1->cpu_utime+t1->cpu_stime > t2->cpu_utime+t2->cpu_stime) {
      return -1;
    } else if (t1->cpu_utime+t1->cpu_stime < t2->cpu_utime+t2->cpu_stime) {
      return 1;
    } else {
      return 0;
    }
  }
}

static topcpu_t top1[MAXTOPCPU]={0};
static topcpu_t top2[MAXTOPCPU]={0};


int get_topcpu(topcpu_t **top, int ntop)
{
  static topcpu_t *topp = top2;
  static topcpu_t *topprev = top1;
  topcpu_t *topswap = NULL;
  struct dirent *dirent;
  int count = 0;
  int i=0;
  struct timeval now;
  static struct timeval old_now={0};
  topswap = topprev;
  topprev = topp;
  topp = topswap;
  gettimeofday(&now, NULL);
  DIR *dir = opendir("/proc");
  // TODO: Allocate the top array here
  while (dirent = readdir(dir)) {
    if (isdigit(dirent->d_name[0])) {
      count++;
    }
  }
  rewinddir(dir);
  while (dirent = readdir(dir)) {
    if (isdigit(dirent->d_name[0])) {
      char procfile[128];
      if (i >= MAXTOPCPU) break;
      sprintf(procfile, "/proc/%s/stat", dirent->d_name);
      FILE *f = fopen(procfile, "r");
      char line[512];
      if (f) {
	fgets(line, sizeof(line)-1, f);
	struct stat stat;
	lstat(procfile, &stat);
	char comm[64];
	char state[2]={0};
	uint pid, ppid, pgrp, session, tty, tpgid, flags, minflt,cminflt, majflt, cmajflt, utime, stime, cutime;
	uint cstime, priority, p0, itrealvalue, starttime, rlim, startcode;
	int nice;
	unsigned long long vsize, rss;
	//            1  2  3  4  5  6  7   8   9 10 11  12  13  14  15  16  17  18   19  20 21   22   23  24 25  26
	sscanf(line, "%d %s %c %d %d %d %d %d %lu %u %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %lu %llu %llu %lu %lu", 
	       //1    2     3     4         5      6         7      8     9      10       11       12            13     14      15     16
	       &pid, comm, state, &ppid, &pgrp, &session, &tty, &tpgid, &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, 
	       // 17        18    19       20    21            22         23      24   25     26
	       &cstime, &priority, &nice, &p0, &itrealvalue, &starttime, &vsize, &rss, &rlim, &startcode);
	//     1    2   3   4     5     6     7    8       9  10       11  12 13   14   15   16   17   18 19 20 21 22          23      24         25        26       28         29          30     31....
	//     1 (init) S   0     1     1     0    -1 4194560 684 19213991 18 1765 276 584 34987 43041 15  0  1  0 10         2084864 160 4294967295 134512640 134544676 3215657616 3215656300 7635970 0 
	// 12728 (mem)  S 4108 12728 4108 34818 12728 4194304 4273       0  0   0   0  169    0     0  19  0  1  0 174388399 18362368 4211 4294967295 134512640 134514076 3213006512 3213005864 11011074 0 0 536870912 0 0 0 0 17 0 0 0 0
	//    1          2 3 4 5 6 7  8     9 10 11 12 13 14   15 16 17 18 19 20 21 22 23 24         25 26
	//    5 (events/0) S 1 1 1 0 -1 41024  0  0  0  0  0 5612  0  0 10 -5  1  0 24  0  0 4294967295  0 0 0 0 0 0 2147483647 65536 0 0 0 0 17 0 0 0 0

	topp[i].uid = stat.st_uid;
	topp[i].pid = pid;
	long long old_cputime = 0;
	int j, found = 0;
	for (j=0; j<count; j++) {
	  if (topp[i].pid == topprev[j].pid) {
	    found = 1;
	    break;
	  }
	}
	if (found) {
	  topp[i].old_cpu_utime = topprev[j].cpu_utime;
	  topp[i].old_cpu_stime = topprev[j].cpu_stime;
	  topp[i].old_pageflt = topprev[j].pageflt;
	}
	topp[i].pri = priority;
	topp[i].nice = nice;
	strcpy(topp[i].progname, comm);
	topp[i].memsize_1k = vsize/1024;
	topp[i].ressize_1k = rss*4;
	topp[i].stat = state[0];
	topp[i].starttime = starttime;
	topp[i].cpu_utime = utime;
	topp[i].cpu_stime = stime;
	topp[i].pageflt = majflt+minflt;
	topp[i].deltapageflt = majflt+minflt-topp[i].old_pageflt;
	long long deltacpu = (topp[i].cpu_utime - topp[i].old_cpu_utime) + (topp[i].cpu_stime - topp[i].old_cpu_stime);
	double delta = 1.0;
	if (old_now.tv_sec == 0) {
	  delta = now.tv_sec - starttime;
	} else {
	  delta = now.tv_sec - 1 - old_now.tv_sec + 1e-6*(now.tv_usec + 1000000 - old_now.tv_usec);
	}
	if (delta == 0.0) delta = 1.0;
	topp[i].cputime_prs = (deltacpu/delta);
	fclose(f);
	i++;
      }
    }
  }
  closedir(dir);
  if (old_now.tv_sec == 0) {
    old_now = now;
  }
  qsort(topp, count, sizeof(topcpu_t), cmp_top);
  *top = topp;
  return i;
}

