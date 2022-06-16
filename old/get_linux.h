#include <sys/types.h>
#include <sys/ioctl.h>

/*
 * /proc/meminfo
 * - MemTotal
 * - MemFree
 * - Buffers
 * - Cached
 * - SwapCached
 * - SwapTotal
 * - SwapFree
 */
struct vmker {
  unsigned int memtotal;
  unsigned int memfree;
  unsigned int buffers;
  unsigned int cached;
  unsigned int swaptotal;
  unsigned int swapfree;
};

/*
 * /proc/stat
 *      user    unice system idle      iowat  irq   softirq ?
 * cpu  4491616 12291 933233 151873119 272519 90108 197749 0
 * - cpu
 * - user
 * - user_nice
 * - system
 * - idle
 * - iowait
 * - irq
 * - softirq
 * 
 * /proc/vmstat
nr_anon_pages 24113
nr_mapped 6586
nr_file_pages 72473
nr_slab 29578
nr_page_table_pages 680
nr_dirty 3
nr_writeback 0
nr_unstable 0
nr_bounce 0
pgpgin 1610969
pgpgout 11607963
pswpin 0
pswpout 30
pgalloc_dma 1043834
pgalloc_dma32 0
pgalloc_normal 79399464
pgalloc_high 0
pgfree 80445916
pgactivate 2399920
pgdeactivate 352387
pgfault 268555959
pgmajfault 8448
pgrefill_dma 23515
pgrefill_dma32 0
pgrefill_normal 679354
pgrefill_high 0
pgsteal_dma 0
pgsteal_dma32 0
pgsteal_normal 0
pgsteal_high 0
pgscan_kswapd_dma 19475
pgscan_kswapd_dma32 0
pgscan_kswapd_normal 514208
pgscan_kswapd_high 0
pgscan_direct_dma 96
pgscan_direct_dma32 0
pgscan_direct_normal 9280
pgscan_direct_high 0
pginodesteal 0
slabs_scanned 758784
kswapd_steal 520093
kswapd_inodesteal 173747
pageoutrun 10169
allocstall 107
pgrotated 74
 *
 */
#define CPU_USER 0
#define CPU_NICE 1
#define CPU_KERNEL 2
#define CPU_IDLE 3
#define CPU_WAIT 4
#define CPU_IRQ 5
#define CPU_SIRQ 6
struct sysinfo {
  unsigned long long cpu[7];
  unsigned int runque;
  unsigned int runocc;
  unsigned long long ctxt; // pswitch
  unsigned int processes;
  unsigned int intr;
  // The following are not implemented in Linux, (sigh AIX 3.1/3.2 had those)
  unsigned int iget;
  unsigned int syscall;
  unsigned int namei;
  unsigned int sysread;
  unsigned int syswrite;
  unsigned int sysfork;
  unsigned int writech;
  unsigned int sysexec;
  unsigned int rawch;
  unsigned int canch;
  unsigned int xmtint;
  unsigned int outch;
  unsigned int dirblk;
  unsigned int readch;
  unsigned int rcvint;
};
struct vminfo {
  unsigned int pgfault;
  unsigned int pageins;
  unsigned int pageouts;
  unsigned int pswpin;
  unsigned int pswpout;
  unsigned int pgexct;
  unsigned int ctxt;
};
struct sysvminfo {
  struct sysinfo sysinfo;
  struct vminfo vminfo;
  struct vmker vmker;
};

/*
 * /proc/diskstats
major minor name rt rm     rs      rtms  wt     wm      ws       tms     ioq ioms   iosqueuems
   8    0 sda 80723 41086 2777195 904808 828575 1679305 20062966 13415296 0 3466576 14320100
 * - dev_major
 * - dev_minor
 * - name
 * - read_ios
 * - read_merges
 * - read_sectors
 * - read_time_ms
 * - write_ios
 * - write_merges
 * - write_sectors
 * - write_time_ms
 * - current_io_pending
 * - io_time_ms
 * - io_time_queue
 * 
 
 */
struct dkstat {
  char diskname[64];
  unsigned int speed; // locally generated
  unsigned int dk_rblks;
  unsigned int dk_wblks;
  unsigned int dk_bsize;
  unsigned int dk_xfers;
  unsigned int dk_time;
  struct dkstat *dknextp;
};

/*
 * /proc/net/dev
 * - ifname:
 * - recv_bytes
 * - recv_pkts
 * - recv_err
 * - recv_drop
 * - recv_fifo
 * - recv_frame
 * - recv_comp
 * - recv_mcast
 * - send_bytes
 * - send_pkts
 * - send_errs
 * - send_drop
 * - send_fifo
 * - send_colls
 * - send_carr
 * - send_comp
 */
#define IFNAMSIZ 16
struct ifnet {
  char if_name[IFNAMSIZ];
  int if_unit;
  unsigned long long if_ibytes;
  unsigned long long if_obytes;
  unsigned long long if_ipackets;
  unsigned long long if_opackets;
  struct ifnet *if_next;
};

struct cpuinfo {
  unsigned long long cpu[5];
  unsigned int runque;
  unsigned int runocc;
  unsigned int ctxt;
  unsigned int iget;
  unsigned int syscall;
  unsigned int namei;
  unsigned int sysread;
  unsigned int syswrite;
  unsigned int sysfork;
  unsigned int writech;
  unsigned int sysexec;
  unsigned int rawch;
  unsigned int canch;
  unsigned int xmtint;
  unsigned int outch;
  unsigned int dirblk;
  unsigned int readch;
  unsigned int rcvint;
};

typedef struct { /* Client rpc: */
  uint calls, badcalls;
  uint retrans; // 
  uint badxid;
  uint timeout;
  uint wait;
  uint newcred;
} nfs_rcstat_t;

typedef struct { /* Server rpc: */
  uint calls, badcalls;
  uint nullrecv;
  uint badlen;
  uint xdrcall;
} nfs_rsstat_t;

typedef struct { /* Client nfs: */
  uint nclsleep;
  uint calls; // 
  uint nclget;
  uint badcalls;
  uint null;
  uint getattr; //
  uint setattr;
  uint lookup; // 
  uint access;
  uint readlink;
  uint read;   //
  uint write;  //
  uint create;
  uint mkdir;
  uint symlink;
  uint mknod;
  uint remove;
  uint rmdir;
  uint rename;
  uint link;
  uint readdir;
  uint readdirplus;
  uint fsstat;
  uint fsinfo;
  uint pathconf;
  uint commit;
} nfs_clstat_t;

typedef struct {/* Server nfs: */
  uint calls;
  uint badcalls;
  uint null;
  uint getattr; //
  uint setattr;
  uint lookup; // 
  uint access;
  uint readlink;
  uint read;   //
  uint write;  //
  uint create;
  uint mkdir;
  uint symlink;
  uint mknod;
  uint remove;
  uint rmdir;
  uint rename;
  uint link;
  uint readdir;
  uint readdirplus;
  uint fsstat;
  uint fsinfo;
  uint pathconf;
  uint commit;
} nfs_svstat_t;

typedef struct {
  nfs_rcstat_t rc;
  nfs_rsstat_t rs;
  nfs_clstat_t cl;
  nfs_svstat_t sv;
} nfsstat_t;

extern int no_nfs;

#define TOPCPU_USERNAME_LEN 16
#define TOPCPU_PROGNAME_LEN 64
#define MAXTOPCPU 2000

typedef struct {                                /*        Size */
  uid_t   uid;                                  /*           4 */
  pid_t   pid;                                  /*           4 */
  u_char  pri;                                  /*           1 */
  char   nice;                                  /*           1 */
  u_char  stat;                                 /*           1 */
  char    username[TOPCPU_USERNAME_LEN];        /*          16 */
  char    progname[TOPCPU_PROGNAME_LEN];        /*          64 */
  u_long  memsize_1k;                           /*           4 */
  u_long  ressize_1k;                           /*           4 */
  u_long  old_pageflt;                              /*           4 */
  u_long  pageflt;                              /*           4 */
  time_t  starttime;                            /*           4 */
  long long old_cpu_utime;                            /*           8 */
  long long old_cpu_stime;                            /*           8 */
  long long cpu_utime;                            /*           8 */
  long long cpu_stime;                            /*           8 */
  u_short   deltapageflt;                       /*           2 */
  double    cputime_prs;                     /* CPU%      8 */
} topcpu_t;                                    

extern int get_sysvminfo(struct sysvminfo **);
extern int get_dkstat(struct dkstat **);
extern int get_ifnet(struct ifnet **);
extern int get_nfsstat(nfsstat_t **);
extern int get_cpuinfo(struct cpuinfo **, int *ncpus);
extern int get_topcpu(topcpu_t **, int ntop);

