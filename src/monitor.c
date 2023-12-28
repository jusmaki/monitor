/* monitor -- Linux and AIX RS/6000 System monitor 
 *
 * Copyright (c) 1991 - 1996, 2011- Jussi Maki. All Rights Reserved.
 * See LICENSE.
 */
/* Author: Jussi Maki
 *
 * email:    jusmaki@gmail.com
 *
 * created:  15.5.1991 v1.0        first release
 *           14.5.1994 v1.12 -0     moved previous history to HISTORY file
 *           15.6.1994 v1.12 -4
 * latest:   18.7.1995 v1.13-b0     support for plain ascii output
 *           31.8.1995 v1.13-b1     cludged support for FDDI in AIX4
 *           16.11.1995 v1.13-b2    bugfixes for the network part (beta)
 *           06.02.1996 v1.14-b0    support for SMP machines (-smp)
 *           25.03.1996 v1.14       time to release the 1.14
 *           04.12.2011 v3.0        Adding Linux support
 *           28.12.2023 v3.1        Improving Linux support
 * TODO: 
 *           - Adding Linux cgroup support for docker monitoring
 */

#define MONITOR_NAME "Linux monitor v3.1:"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <locale.h>

#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pwd.h>
#include <stdarg.h>
#include <string.h>
#ifdef AIX
#include "get_nfsstat.h"
#include "get_topcpu.h"
#include "get_sysvminfo.h"
#include "get_dkstat.h"
#include "get_ifnet.h"
#include "get_cpuinfo.h"
#include "getloadavg.h"
#else
#include <termios.h>
#include <sys/ioctl.h>
#include "get_linux.h"
#endif

#define MONITOR_NTOP_DEFAULT 16
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

double realtime();
void sighandler(int signum);
void print_sysinfo(double refresh_time, 
                   struct sysvminfo *sv1, struct sysvminfo *sv2);
int mon_print(int y, int x, const char *fmt, ...);
void display_help();
void print_cpuinfo(double rs, struct cpuinfo **cpus, int n_cpus, int ci, topcpu_t *top, int ntop);
static void getscreensize(int *cols, int *rows, int *not_a_tty);
static void parse_args(int argc, char **argv);
static void print_summary(double refresh_time,struct sysvminfo *sv1,struct sysvminfo *sv2);
static void print_ifnet_full(double refresh_time,struct ifnet *if1, struct ifnet *if2);
static void print_dkstat(double refresh_time,struct dkstat *dk1,struct dkstat *dk2);
static void print_dkstat_full(double refresh_time,struct dkstat *dk1,struct dkstat *dk2);
static void print_ifnet(double refresh_time,struct ifnet *if1,struct ifnet *if2);
static void print_nfsstat(double refresh_time, nfsstat_t *n1, nfsstat_t *n2);
static void print_topcpu(double refresh_time, topcpu_t *top, int ntop);
static void wait_input_or_sleep(int secs);
static void checkinput();
static void quit_monitor();
int bytesreadable(int in);
static void strchgen(char *str, char ch, int len);
static int summary_dkstat(struct dkstat **sump, struct dkstat *dk1, struct dkstat *dk2, int *active);


int topflag_showusername;
int topflag_usersystemtime;
int dk_cnt;  /* number of disks in system  */
int sleep_sec=10;
int end_monitor=0;
int monflag_show_nfs=0;
int g_ntop = 17; /* top procs to show by default */
int show_all=0,show_top=0;
int show_disk_full=0;
int show_net_full=0;
int show_smp=0;
int columns,lines=24;
int show_top_running=0;
int plain_ascii=0;
int g_count=-1;


int main(int argc,char *argv[])
{
    double update_time[2];
    double refresh_time;
    struct sysvminfo *sv[2]={0};
    struct dkstat    *dk[2]={0};
    struct ifnet     *ifnet[2]={0};
    struct cpuinfo   *cpus[2]={NULL,NULL}; int n_cpus;
    nfsstat_t        *nfs[2]={0};
    topcpu_t         *topcpu = NULL;
    int ntop_found;
    int s_old=0, s_new=0;

    topflag_showusername=1;
    setlocale(LC_NUMERIC,"C"); /* be sure that decimal-point is "." */
    getscreensize(&columns,&lines,&plain_ascii);
    if (columns > 94) topflag_usersystemtime=1; 
    g_ntop = lines - 6; /* for default ntop leave space for display */
    parse_args(argc, argv); /* this will also set the ntop if '-top NUM' */
    g_ntop = min(g_ntop, MAXTOPCPU);

    /*signal(SIGTSTP,sighandler);*/
    /*    signal(SIGINT,sighandler);*/
    /*    signal(SIGQUIT,sighandler);*/

    if (!plain_ascii) { initscr(); clear(); cbreak(); }

    if (show_top) ntop_found = get_topcpu(&topcpu, g_ntop);
    update_time[s_new] = realtime();
    get_sysvminfo(&sv[s_new]);
    get_dkstat(&dk[s_new]);
    get_ifnet(&ifnet[s_new]);
    get_nfsstat(&nfs[s_new]);
    get_cpuinfo(&cpus[s_new], &n_cpus);
    s_old=s_new; s_new = (s_new+1)&1;
    if (g_count==0) {
        g_count=1;
        usleep(100000);
    } else {
        sleep(1);
    }

    while (! end_monitor) {
        get_sysvminfo(&sv[s_new]);
        get_dkstat(&dk[s_new]);
        get_ifnet(&ifnet[s_new]);
        get_nfsstat(&nfs[s_new]);
        get_cpuinfo(&cpus[s_new], &n_cpus);
        if (show_top||show_smp) ntop_found = get_topcpu(&topcpu, g_ntop);
        update_time[s_new] = realtime();
        refresh_time = update_time[s_new] - update_time[s_old];
        if (show_smp) {
            print_summary(refresh_time, sv[s_new],     sv[s_old]);
            print_cpuinfo(refresh_time, cpus, n_cpus, s_old, topcpu, ntop_found);
        } else if (show_net_full) {
            print_summary(refresh_time, sv[s_new],     sv[s_old]);
            print_ifnet_full(refresh_time, ifnet[s_new], ifnet[s_old]);
        } else if (show_disk_full) {
            print_summary(refresh_time, sv[s_new],     sv[s_old]);
            print_dkstat_full(refresh_time, dk[s_new], dk[s_old]);
        } else if (show_top && !show_all) {
            print_summary(refresh_time, sv[s_new],     sv[s_old]);
        } else {
            print_sysinfo(refresh_time, sv[s_new],     sv[s_old]);
            print_ifnet(refresh_time,   ifnet[s_new],  ifnet[s_old]);
            print_dkstat(refresh_time,  dk[s_new],     dk[s_old]);
            print_nfsstat(refresh_time, nfs[s_new],    nfs[s_old]);
        } 
        if (show_top) {
            print_topcpu(refresh_time, topcpu, ntop_found);
        }
        if (!plain_ascii) { move(0,0); refresh(); }
        s_old=s_new; s_new = (s_new+1)&1;
        if (--g_count == 0) break;
        wait_input_or_sleep(sleep_sec);
        checkinput();
    }
    if (!plain_ascii) {
        endwin();
    }
}

static void parse_args(int argc, char **argv)
{
    while (argc>1) {
        if (argv[1][0] == '-') {
            switch(argv[1][1]) {
            case 's':
                if (strcmp(argv[1], "-smp")==0) {
                    show_smp=1;
                } else {
                    sleep_sec = atoi(argv[2]); argc--;argv++;
                    if (sleep_sec < 1) sleep_sec=1;
                }
                break;
            case 'p': /* plain_ascii */
                plain_ascii=1;
                lines = 40000; /* no limits on line length */
                break;
            case 'a': /* -all */
                show_all = 1;  show_top = 1;
                g_ntop = lines-27;
                if (g_ntop < 0) show_top = 0;
                break;
            case 't': /* -top [nproc] */
                show_top = 1;
                if (argc > 2 && atoi(argv[2])>0) {
                    g_ntop=atoi(argv[2]);
                    argc--;argv++;
                } else {
                    g_ntop = MONITOR_NTOP_DEFAULT;
                }
                break;
            case 'r':
                show_top_running=1;
                break;
            case 'u': /* -u   ... dont show user names in top */
                topflag_showusername=0;
                break;
            case 'd': /* -d[isk] */
                show_disk_full=1;
                break;
            case 'n': /* -n[et] */
                show_net_full=1;
                break;
            case 'c': /* -c[ount] count */
                g_count=atoi(argv[2]); argc--;argv++;
                break;
            default:
                fprintf(stderr,"Usage: monitor [-s sleep] [-top [nproc]] [-u] [-all] [-d]\n");
                fprintf(stderr,"Monitor is a curses-based *nix system event monitor\n");
                fprintf(stderr,"     -s sleep      set refresh time\n");
                fprintf(stderr,"     -top [nproc]  show top processes sorted by cpu-usage\n");
                fprintf(stderr,"     -u            don't show usernames in top display\n");
                fprintf(stderr,"     -all          show all variable (needs high window)\n");
                fprintf(stderr,"     -disk         show more on disks\n");
                fprintf(stderr,"     -net          show more on network\n");
                fprintf(stderr,"     -smp          show SMP (multiprocessor) info\n");
                fprintf(stderr,"     -p            show plain ascii output\n");
                fprintf(stderr,"     -c count      show count times\n");
                exit(0);
                break;
            }
        }
        argv++;argc--;
    }
}

/* wait for terminal input with secs-timeout */
static void wait_input_or_sleep(int secs)
{
    fd_set input_fd;
    struct timeval timeout;

    FD_ZERO(&input_fd);
    FD_SET(fileno(stdin),&input_fd);
    timeout.tv_sec=secs; timeout.tv_usec=0;
    select(fileno(stdin)+1,&input_fd,0,0,&timeout);
}

static void getscreensize(int *cols, int *rows, int *not_a_tty)
{
    if (isatty(fileno(stdout))) {
#ifdef AIX
        *cols = atoi((char *)termdef(fileno(stdout),'c')); /* c like columns */
        *rows = atoi((char *)termdef(fileno(stdout),'l')); /* l like lines */
#else
#ifdef TIOCGWINSZ
        struct winsize winsize;
        int rc = ioctl(0, TIOCGWINSZ, &winsize);
        *cols = winsize.ws_col;
        *rows = winsize.ws_row;
#endif
#endif
        if (*cols==0) *cols=80;
        if (*rows==0) *rows=24;
        *not_a_tty=0;
    } else {
        *not_a_tty=1;
        *cols=100;
        *rows=40000;
    }
}

double realtime()
{
    struct timeval tp;
    gettimeofday(&tp,0);
    return((double)tp.tv_sec+tp.tv_usec*1.0e-6);
}

static void quit_monitor()
{
    if (!plain_ascii) {
        nocbreak();
        endwin();
    }
    exit(0);
}

/* checkinput is the subroutine to handle user input */
static void checkinput()
{
    char inbuf[1024];
    int nbytes;
    int inbytes;

    if ((nbytes=bytesreadable(fileno(stdin))) > 0) {
        inbytes = read(fileno(stdin),inbuf,nbytes);
        if (inbuf[0]=='q' || inbytes == 0 || inbytes == -1) {
            quit_monitor();
        }
        switch (inbuf[0]) {
        case 12:  /* cltr-L ^L */
            clear();     
            break;
        case 's': /* show smp */
            show_smp = (show_smp+1)&1;
            clear();
            break;
        case 'd': /* toggle show disk full */
            show_disk_full = (show_disk_full+1)&1;
            clear();
            break;
        case 'n': /* toggle show net full */
            show_net_full = (show_net_full+1)&1;
            clear();
            break;
        case 't':
            show_top = (show_top+1)&1;
            clear();
            break;
        case 'h':
        case '?':
            display_help();
            while (bytesreadable(fileno(stdin))==0) usleep(200000);
            read(fileno(stdin),inbuf,1);
            clear();
            break;
        }
    }
}

void display_help()
{
    clear();
    printw("monitor -- *nix %s\n",MONITOR_NAME);
    printw("(C) Copyright 1991-1996,2011- Jussi Maki;  Email: jusmaki@gmail.com\n");
    printw("\n");
    printw("Monitor displayes various *nix utilization characterics.\n");
    printw("Following command line arguments and commands are available:\n");
  
    printw("     -s sleep      set refresh time\n");
    printw("     -top [nproc]  show top processes sorted by cpu-usage\n");
    printw("     -u            don't show usernames in top display\n");
    printw("     -all          show all variable (needs high window)\n");
    printw("     -disk         show more on disks\n");
    printw("     -net          show more on network\n");
    printw("     -smp          show SMP (multiprocessor) info\n");
    printw("     -p            show plain ascii output\n");
    printw("     -c count      show count times\n");
    printw("\n");
    printw("Following character commands are available in full screen mode:\n");
    printw("    ^L             refresh the screen\n");
    printw("     s             display SMP multiprosessor cpuinfo\n");
    printw("     t             display top cpu processes\n");
    printw("     n             display detailed network info\n");
    printw("     d             display detailed disk info\n");
    printw("\n");
    printw("Press any key to continue.\n");
    refresh();
}

int bytesreadable(int in)
{
    static int bytes;
    ioctl(in,FIONREAD,&bytes);
    return(bytes);
}

/**********************************************************************/

#define BARLEN 72
#define SIDELTA(a) (si->a - si2->a)
#define VMDELTA(a) (vm->a - vm2->a)


void print_sysinfo(double refresh_time, 
                   struct sysvminfo *sv1, struct sysvminfo *sv2)
{
    double cpu_sum;
    double str_cpu_sum;
    double swp_proc;
    double runnable,runque_tmp, runocc_tmp;
    char bar[BARLEN];
    time_t time1;
    double loadv[3];
    char hostnm[128];
    int x,y;
    struct sysinfo *si,*si2;
    struct vmker *vmk;
    struct vminfo *vm,*vm2;
    char *timestrp;

    si = &sv1->sysinfo;
    si2= &sv2->sysinfo;
    vm = &sv1->vminfo;
    vm2= &sv2->vminfo;
    vmk= &sv1->vmker;

    gethostname(hostnm,sizeof(hostnm));
    time1=time(0);
    timestrp = ctime(&time1);
    timestrp[strlen(timestrp)-1]='\0'; /* remove last \n */
    mon_print(0,0, "%-19s %-31s %s",MONITOR_NAME,hostnm,timestrp);
    cpu_sum = SIDELTA(cpu[CPU_IDLE]) + SIDELTA(cpu[CPU_USER]) 
        + SIDELTA(cpu[CPU_KERNEL]) + SIDELTA(cpu[CPU_WAIT]);
    str_cpu_sum = cpu_sum/(double)BARLEN;
    cpu_sum = cpu_sum/100.0;

    mon_print(1,0, "Sys %4.1lf%% Wait %4.1lf%% User %4.1lf%% Idle %5.1lf%%           Refresh: %5.2f s",
              SIDELTA(cpu[CPU_KERNEL])/cpu_sum,
              SIDELTA(cpu[CPU_WAIT])/cpu_sum,
              SIDELTA(cpu[CPU_USER])/cpu_sum,
              SIDELTA(cpu[CPU_IDLE])/cpu_sum, 
              refresh_time);
    mon_print(2,0,"0%%             25%%              50%%               75%%              100%%");
    bar[0]=0;
    strchgen(bar,'=',(int)(SIDELTA(cpu[CPU_KERNEL])/str_cpu_sum));
    strchgen(bar,'W',(int)(SIDELTA(cpu[CPU_WAIT])/str_cpu_sum));
    strchgen(bar,'>',(int)(SIDELTA(cpu[CPU_USER])/str_cpu_sum));
    strchgen(bar,'.',(int)(SIDELTA(cpu[CPU_IDLE])/str_cpu_sum));
    if (!plain_ascii) { move(3,0); clrtoeol(); }
    mon_print(3,0,"%s",bar);

    getloadavg(loadv,3);
    runque_tmp = (double) SIDELTA(runque);
    runocc_tmp = (double) SIDELTA(runocc);
    if(runocc_tmp==0.0)
        runnable=0.0;
    else
        runnable=runque_tmp/runocc_tmp-1.0;
    mon_print(4,0,"Runnable processes %5.2lf load average: %5.2lf, %5.2lf, %5.2lf\n",
              (runnable+1.0)*SIDELTA(runocc)/refresh_time, loadv[0],loadv[1],loadv[2]);
    x=0;y=6;
    mon_print(y+0,x, "Memory    Real     Virtual     Paging (4kB)");
    mon_print(y+1,x, "free   %8.0lf MB %7.0lf MB  %6.0f pgfaults",
              vmk->memfree/1024.0,vmk->swapfree/1024.0,
              VMDELTA(pgfault)/refresh_time);
    mon_print(y+2,x, "procs  %8.0lf MB %7.0lf MB   %5.0f pgin", 
              (vmk->memtotal - vmk->memfree - vmk->buffers - vmk->cached)/1024.0,
              (vmk->swaptotal - vmk->swapfree)/1024.0,
              VMDELTA(pageins)/refresh_time);
    mon_print(y+3,x, "cached %8.0lf MB              %5.0f pgout", 
              vmk->cached/1024.0,
              VMDELTA(pageouts)/refresh_time);
    mon_print(y+4,x, "buffs  %8.0lf MB              %5.0f pgsin", 
              vmk->buffers/1024.0,
              VMDELTA(pswpin)/refresh_time);
    mon_print(y+5,x, "total  %8.0lf MB %7.0lf MB   %5.0f pgsout",
              (vmk->memtotal)/1024.0, vmk->swaptotal/1024.0,
              VMDELTA(pswpout)/refresh_time);

    x=49;y=7;
    mon_print(y+0,x-2, "Process events");
    mon_print(y+1,x, "%5.0f ctxt", 
              SIDELTA(ctxt)/refresh_time);
    mon_print(y+2,x, "%5.0f forks ", 
              SIDELTA(processes)/refresh_time);
    mon_print(y+3,x, "%5.0f intr",  
              SIDELTA(intr)/refresh_time);
    
}

static void print_summary(double refresh_time,struct sysvminfo *sv1,struct sysvminfo *sv2)
{
    time_t time1;
    double loadv[3];
    char hostnm[128];
    double cpu_sum;
    struct sysinfo *si,*si2;
    struct vmker *vmk;
    struct vminfo *vm,*vm2;
    char  *timestrp;

    si = &sv1->sysinfo;
    si2= &sv2->sysinfo;
    vm = &sv1->vminfo;
    vm2= &sv2->vminfo;
    vmk= &sv1->vmker;

    /* hostname loadavg date-time */
    /* cpu-states */
    /* memory-states */
    gethostname(hostnm,sizeof(hostnm));
    getloadavg(loadv,3);
    time1=time(0);
    timestrp = ctime(&time1);
    timestrp[strlen(timestrp)-1]='\0'; /* remove last \n */
    cpu_sum = SIDELTA(cpu[CPU_IDLE]) + SIDELTA(cpu[CPU_USER]) 
        + SIDELTA(cpu[CPU_KERNEL]) + SIDELTA(cpu[CPU_WAIT]);
    cpu_sum = cpu_sum/100.0;
    mon_print(0,0, "%-16s load averages: %5.2lf, %5.2lf, %5.2lf   %s",
              hostnm,
              loadv[0],loadv[1],loadv[2],
              timestrp);
    mon_print(1,0,"Cpu states:     %5.1f%% user, %5.1f%% system, %5.1f%% wait, %5.1f%% idle",
              SIDELTA(cpu[CPU_USER])/cpu_sum,
              SIDELTA(cpu[CPU_KERNEL])/cpu_sum,
              SIDELTA(cpu[CPU_WAIT])/cpu_sum,
              SIDELTA(cpu[CPU_IDLE])/cpu_sum);
    mon_print(2,0, "Real memory:    %6.1lfM free %6.1lfM procs %6.1lfM cached %6.1lfM total",
              vmk->memfree/1024.0,
              (vmk->memtotal - vmk->memfree - vmk->buffers - vmk->cached)/1024.0, 
              vmk->cached/1024.0,
              vmk->memtotal/1024.0);
    mon_print(3,0, "Virtual memory: %6.1lfM free %6.1lfM used                %6.1lfM total",
              vmk->swapfree/1024.0,
              (vmk->swaptotal-vmk->swapfree)/1024.0, 
              vmk->swaptotal/1024.0);
}

#define dkD(a)  ((dk1->a) - (dk2->a))

typedef struct dksort {
    struct dkstat *dk1,*dk2;
    unsigned long speed;
} dksort_t;

int cmp_dksort(const void *a, const void *b)
{
    if (((dksort_t *)a)->speed > ((dksort_t *)b)->speed) return (-1);
    else if (((dksort_t *)a)->speed < ((dksort_t *)b)->speed) return ( 1);
    return (0);
}

static void print_dkstat(double refresh_time,struct dkstat *dk1,struct dkstat *dk2)
{
    int i;
    int x,y;
    static struct dkstat *dks=NULL;
    static dksort_t *dksorted;
    static int ndisks_sorted=0;
    int ndisks, active;
    char active_str[16];
    
    ndisks = summary_dkstat(&dks, dk1,dk2, &active);
    if (ndisks_sorted == 0) {
        dksorted = (dksort_t *) malloc(sizeof(dksort_t)*ndisks);
        ndisks_sorted = ndisks;
    }
    if (ndisks > ndisks_sorted) { 
        dksorted = (dksort_t *) realloc(dksorted, sizeof(dksort_t)*ndisks);
        ndisks_sorted = ndisks;
    }
      
    x=0;y=13;
    mon_print(y  ,x, "DiskIO     Total Summary");
    mon_print(y+1,x, "read      %7.1f MB/s",
              dks->dk_rblks*dks->dk_bsize/1024.0/1024.0/refresh_time);
    mon_print(y+2,x, "write     %7.1f MB/s",
              dks->dk_wblks*dks->dk_bsize/1024.0/1024.0/refresh_time);
    mon_print(y+3,x, "transfers %7.0f tps",dks->dk_xfers/refresh_time);
    sprintf(active_str, "%d/%d", active,ndisks);
    mon_print(y+4,x, "active    %7s disks",active_str);

    i=0;
    while (dk1) { /* initialize the sorting list */
        dksorted[i].dk1 = dk1;
        dksorted[i].dk2 = dk2;
        dksorted[i].speed = dkD(dk_rblks)+dkD(dk_wblks);
        if (! dk1->dknextp) break;
        dk1 = dk1->dknextp;
        dk2 = dk2->dknextp;
        i++;
    }
    qsort((void *)dksorted, (size_t)ndisks, (size_t)sizeof(dksort_t), cmp_dksort);
    x=0;y+=6;
    mon_print(y+0,x, "TOPdisk  read  write     busy");
    i = 0;
    for (i=0; y+i+1<lines && i<min(10,ndisks); i++) {
        dk1 = dksorted[i].dk1;
        dk2 = dksorted[i].dk2;
        mon_print(y+1+i,x, "%-7s %5.1f %5.1f MB/s %3.0f%%",
                  dk1->diskname,
                  dkD(dk_rblks)*dk1->dk_bsize/1024.0/1024.0/refresh_time,
                  dkD(dk_wblks)*dk1->dk_bsize/1024.0/1024.0/refresh_time,
                  dkD(dk_time)/refresh_time);
    }
    mon_print(y+((i+2)<11?11:i+2),x,"");
}

static void print_dkstat_full(double refresh_time,struct dkstat *dk1,struct dkstat *dk2)
{
    int i;
    int x,y;
    double rsize,wsize,xfers;
    static struct dkstat *dks=NULL;
    int ndisks, active;
    
    ndisks = summary_dkstat(&dks, dk1,dk2, &active);
    x=0;y=5;
    mon_print(y+0,x, "DiskIO    read    write    rsize  wsize xfers  busy");
    i = 0;
    while (1) {
        move(y+1+i,x);
        if (y+1+i>=lines) {
            printw("...more disks with bigger window...");
            break;
        }
        xfers = dkD(dk_xfers);
        if (xfers == 0) {
            rsize = wsize = 0;
        } else {
            rsize = dkD(dk_rblks)*dk1->dk_bsize/1024.0/xfers;
            wsize = dkD(dk_wblks)*dk1->dk_bsize/1024.0/xfers;
        }
        mon_print(y+1+i,x, "%-7s %6.1f %6.1f MB/s %4.0f %4.0f kB %6.0f %3.0f%%",
                  dk1->diskname,
                  dkD(dk_rblks)*dk1->dk_bsize/1024.0/1024.0/refresh_time,
                  dkD(dk_wblks)*dk1->dk_bsize/1024.0/1024.0/refresh_time,
                  rsize, wsize, xfers/refresh_time,
                  dkD(dk_time)/refresh_time);
        if (!dk1->dknextp) break;
        dk1 = dk1->dknextp;
        dk2 = dk2->dknextp;
        i++;
    }
    x=53;
    mon_print(y, x, "Summary    Total");
    mon_print(y+1,x, "read     %7.1f kbyte/s", 
              dks->dk_rblks*dks->dk_bsize/1024.0/refresh_time);
    mon_print(y+2,x, "write    %7.1f kbyte/s", 
              dks->dk_wblks*dks->dk_bsize/1024.0/refresh_time);
    mon_print(y+3,x, "xfers    %7.1f tps",dks->dk_xfers/refresh_time);
    mon_print(y+4,x, "busy     %7.1f %%",dks->dk_time/refresh_time);
    mon_print(y+6,x, "active   %7d disks", active);
    mon_print(y+7,x, "total    %7d disks",ndisks);
    move(y+i+2,0);
}

#define ifD(a) ((if1->a) - (if2->a))
#define ifD2(if1, if2, a) (((if1)->a) - ((if2)->a))

typedef struct ifsort {
    struct ifnet *if1,*if2;
    unsigned long speed;
} ifsort_t;

int cmp_ifsort(const void *a, const void *b)
{
    if (((ifsort_t *)a)->speed > ((ifsort_t *)b)->speed) return (-1);
    else if (((ifsort_t *)a)->speed < ((ifsort_t *)b)->speed) return ( 1);
    return (0);
}

static void print_ifnet(double refresh_time,struct ifnet *if1,struct ifnet *if2)
{
    int i;
    int x,y;
    char ifname[IFNAMSIZ*2];
    static ifsort_t *ifsorted;
    static int nif_sorted = 0;
    int nif = 0;

    for (nif = 0; nif < MAXIF; nif++) {
        if (if1[nif].if_next == NULL) break;
    }
    nif++;
    
    if (nif_sorted == 0) {
        ifsorted = (ifsort_t *)malloc(sizeof(ifsort_t)*nif);
        nif_sorted = nif;
    }
    if (nif > nif_sorted) {
        free(ifsorted);
        ifsorted = (ifsort_t *)malloc(sizeof(ifsort_t)*nif);
        nif_sorted = nif;
    }
    
    i=0;
    while (if1) { /* initialize the sorting list */
        ifsorted[i].if1 = &if1[i];
        ifsorted[i].if2 = &if2[i];
        ifsorted[i].speed = ifD2(&if1[i],&if2[i],if_ibytes)+ifD2(&if1[i],&if2[i],if_obytes);
        if (! if1[i].if_next) break;
        i++;
    }
    qsort((void*)ifsorted, (size_t)nif, (size_t)sizeof(ifsort_t), cmp_ifsort);
    
    i=0;
    x=54;y=13;
    mon_print(y+0,x, "Netw       read  write");
    for (int i=0; i<nif; i++) {
        if (i==20) break;
        if1 = ifsorted[i].if1;
        if2 = ifsorted[i].if2;
        if (if1->if_unit == -1) 
            sprintf(ifname, "%s",if1->if_name);
        else
            sprintf(ifname, "%s%d", if1->if_name, if1->if_unit);
        mon_print(y+1+i,x, "%-8s %6.1f %6.1f MBit/s",
                  ifname,
                  ifD(if_ibytes)*8.0/1024.0/1024.0/refresh_time,
                  ifD(if_obytes)*8.0/1024.0/1024.0/refresh_time);
    }
}


static void print_ifnet_full(double refresh_time,struct ifnet *if1, struct ifnet *if2)
{
    int i;
    int x,y;
    char ifname[IFNAMSIZ*2];
    static ifsort_t *ifsorted;
    static int nif_sorted = 0;
    int nif = 0;

    for (nif = 0; nif < MAXIF; nif++) {
        if (if1[nif].if_next == NULL) break;
    }
    nif++;

    if (nif_sorted == 0) {
        ifsorted = (ifsort_t *)malloc(sizeof(ifsort_t)*nif);
        nif_sorted = nif;
    }
    if (nif > nif_sorted) {
        free(ifsorted);
        ifsorted = (ifsort_t *)malloc(sizeof(ifsort_t)*nif);
        nif_sorted = nif;
    }
    
    i=0;
    while (if1) { /* initialize the sorting list */
        ifsorted[i].if1 = &if1[i];
        ifsorted[i].if2 = &if2[i];
        ifsorted[i].speed = ifD2(&if1[i],&if2[i],if_ibytes)+ifD2(&if1[i],&if2[i],if_obytes);
        if (! if1[i].if_next) break;
        i++;
    }
    qsort((void*)ifsorted, (size_t)nif, (size_t)sizeof(ifsort_t), cmp_ifsort);
    
    i=0;
    x=0;y=6;
    mon_print(y+0,x, "Netw       read   write        rcount wcount rsize wsize\n");
    for (i = 0; i<nif; i++) {
        if (i==20) break;
        if1 = ifsorted[i].if1;
        if2 = ifsorted[i].if2;
        int isize,osize;
        if (ifD(if_ipackets)) isize = ifD(if_ibytes)/ifD(if_ipackets);
        else isize = 0;
        if (ifD(if_opackets)) osize = ifD(if_obytes)/ifD(if_opackets);
        else osize = 0;
        if (if1->if_unit == -1) 
            sprintf(ifname, "%s",if1->if_name);
        else
            sprintf(ifname, "%s%d",if1->if_name, if1->if_unit);
        mon_print(y+1+i,x, "%-8s %6.1f %6.1f MBit/s %6d %6d %5d %5d",
                  ifname,
                  ifD(if_ibytes)*8.0/1024.0/1024.0/refresh_time,
                  ifD(if_obytes)*8.0/1024.0/1024.0/refresh_time,
                  ifD(if_ipackets), ifD(if_opackets),isize, osize);

	       
    }
    /*    printw("\n");*/
}

#define NFSclD(a) (n1->cl.a - n2->cl.a)
#define NFSsvD(a) (n1->sv.a - n2->sv.a)

static void print_nfsstat(double refresh_time, nfsstat_t *n1, nfsstat_t *n2)
{
    int x,y,client_other, server_other;
    double r;
    if (no_nfs) return;
    r = refresh_time;
    x=30; y=13;
    client_other = NFSclD(null)+NFSclD(setattr)+NFSclD(readdirplus)+NFSclD(readlink)
        + NFSclD(create)+NFSclD(remove)+NFSclD(rename)+NFSclD(link)+NFSclD(symlink)
        + NFSclD(mkdir)+NFSclD(rmdir)+NFSclD(readdir)+NFSclD(fsstat);

    server_other = NFSsvD(null)+NFSsvD(setattr)+NFSsvD(readdirplus)+NFSsvD(readlink)
        + NFSsvD(create)+NFSsvD(remove)+NFSsvD(rename)+NFSsvD(link)+NFSsvD(symlink)
        + NFSsvD(mkdir)+NFSsvD(rmdir)+NFSsvD(readdir)+NFSsvD(fsstat);

    mon_print(y+0,x, "Client Server NFS/s");
    mon_print(y+1,x, "%6.1f %6.1f calls", NFSclD(calls)/r, NFSsvD(calls)/r);
    mon_print(y+2,x, "%6.1f %6.1f retry", (n1->rc.retrans-n2->rc.retrans)/r, 0);
    mon_print(y+3,x, "%6.1f %6.1f getattr", NFSclD(getattr)/r, NFSsvD(getattr)/r);
    mon_print(y+4,x, "%6.1f %6.1f lookup",  NFSclD(lookup)/r, NFSsvD(lookup)/r);
    mon_print(y+5,x, "%6.1f %6.1f read",  NFSclD(read)/r, NFSsvD(read)/r);
    mon_print(y+6,x, "%6.1f %6.1f write", NFSclD(write)/r, NFSsvD(write)/r);
    mon_print(y+7,x, "%6.1f %6.1f other", client_other/r, server_other/r);
    mon_print(y+8,0, "");
}

/* generate 'len' characters 'ch' in the end of string 'str' */
static void strchgen(char *str, char ch, int len)
{
    while (*str != 0) str++;
    while (len) { len--; *str++ = ch; }
    *str = 0;
}

void sighandler(int signum)
{
    switch (signum) {
    case SIGTSTP:
        if (!plain_ascii) {
            nocbreak();
            endwin();
        }
        kill(getpid(),SIGSTOP);
        /* returned here from sigstop */
        if (!plain_ascii) {
            cbreak();
            initscr();
            clear();
        }
        break;
    case SIGINT:
        if (!plain_ascii) {
            nocbreak();
            endwin();
        }
        exit(0);
        break;
    default: fprintf(stderr,"unknown signal %d\n",signum); fflush(stderr);
        if (!plain_ascii) {
            nocbreak();
        }
        endwin();
        exit(0);
    }
    signal(signum,sighandler);
}

char *statstr[] = {"?","sleep","?","run","T","zombie","sleep"};

void print_mem(char *str, long long mem)
{
    // 9999
    // 99.9k
    // 9999k
    // 99.9M
    // 9999M
    // 99.9G
    // 9999G
    if (mem < 10000) {
        sprintf(str, "%4lld ", mem);
    } else if (mem < 100*1024) {
        sprintf(str, "%4.1fk", mem/1024.0);
    } else if (mem < 10000*1024) {
        sprintf(str, "%4.0fk", mem/1024.0);
    } else if (mem < 100*1024*1024LL) {
        sprintf(str, "%4.1fM", mem/1024.0/1024.0);
    } else if (mem < 10000*1024*1024LL) {
        sprintf(str, "%4.0fM", mem/1024.0/1024.0);
    } else if (mem < 100*1024*1024*1024LL) {
        sprintf(str, "%4.1fG", mem/1024.0/1024.0/1024.0);
    } else {
        sprintf(str, "%4.0fG", mem/1024.0/1024.0/1024.0);
    }
}

static void print_topcpu(double refresh_time, topcpu_t *top, int ntop)
{
    double cputime;
    int i;
    int x,y;
    struct passwd *passwd;
    char username[16];
    static int maxtop=0,old_maxtop=0;

    if (plain_ascii) y=x=0;
    else getyx(stdscr, y, x);
    y+=2;
    if (!topflag_usersystemtime) {
        mon_print(y,0, "    PID USER     PRI NICE  SIZE  RES ST   TIME   CPU%% COMMAND");
        y++;
        for (i=0; i<ntop && y+i+2<lines; i++) {
            if (show_top_running && !top[i].cputime_prs) break;

            cputime = (top[i].cpu_utime + top[i].cpu_stime)/100.0;
            if (topflag_showusername) {
                passwd = getpwuid(top[i].uid);
                strcpy(username, passwd->pw_name);
            } else {
                sprintf(username,"%d",top[i].uid);
            }
            char vsize[32];
            char rsize[32];
            print_mem(vsize, top[i].memsize_1k*1024);
            print_mem(rsize, top[i].ressize_1k*1024);

            mon_print(y+i,0, "%7d %-9s %3d %3d %s %s %c %6.2f %5.1f%% %s\n",
                      top[i].pid,	   username,
                      top[i].pri,	   top[i].nice,
                      vsize,    rsize,
                      top[i].stat,
                      cputime,
                      top[i].cputime_prs,
                      top[i].progname);
        }
    } else {
        mon_print(y,0,"    PID USER     PRI NICE  SIZE   RES  PGFLT ST  USERTIME    SYSTIME   CPU%% COMMAND");y++;
        for (i=0; i<ntop && y+i+3<lines; i++) {
            if (show_top_running && !top[i].cputime_prs) break;

            cputime = top[i].cpu_utime + top[i].cpu_stime;
            if (topflag_showusername) {
                passwd = getpwuid(top[i].uid);
                strcpy(username, passwd->pw_name);
            } else {
                sprintf(username,"%d",top[i].uid);
            }
            cputime = top[i].cpu_utime + top[i].cpu_stime;
            char vsize[32];
            char rsize[32];
            print_mem(vsize, top[i].memsize_1k*1024);
            print_mem(rsize, top[i].ressize_1k*1024);
            mon_print(y+i,0, "%7d %-8s %3d %3d  %s %s %6.0f %c %4d:%02d.%02d %4d:%02d.%02d %5.1f%% %s\n",
                      top[i].pid,	   // %6d
                      username,          // %-8d
                      top[i].pri,	   // %3d
                      top[i].nice,    // 3d
                      vsize, //
                      rsize,
                      top[i].deltapageflt/refresh_time,
                      top[i].stat,
                      (u_long)((top[i].cpu_utime/100)/60),(u_long)((top[i].cpu_utime/100)%60), (u_long)(top[i].cpu_utime%100),
                      (u_long)((top[i].cpu_stime/100)/60),(u_long)((top[i].cpu_stime/100)%60), (u_long)(top[i].cpu_stime%100),
                      top[i].cputime_prs,
                      top[i].progname);
        }
    }
    mon_print(y+i,0,"\n");
    if (!plain_ascii) { /* clear the bottom of screen */
        old_maxtop=maxtop;
        maxtop=i;
        for (i=maxtop; i<old_maxtop; i++) {
            printw("\n");
        }
    }
}

int mon_print(int y, int x, const char *fmt, ...)
{
    char buf[4096];
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vsprintf(buf, fmt, ap);
    va_end(ap);
    if (plain_ascii) {
        fputs(buf,stdout);
        if (buf[strlen(buf)-1]!='\n') fprintf(stdout,"\n");
        fflush(stdout);
    } else {
        move (y,x);
        addstr(buf);
    }
    return(ret);
}

void print_cpuinfo(double rs, struct cpuinfo **cpus, int n_cpus, int ci, topcpu_t *top, int ntop)
{
    int i,t;
    int cp;
    double c_idle,c_user,c_kern,c_wait,c_sum;

    struct cpuinfo cpusum;
    cp = ci^1;
    //    mon_print(5,0,"CPU USER KERN WAIT IDLE%% PSW SYSCALL WRITE  READ WRITEkb  READkb\n");  
    mon_print(5,0,"CPU USER KERN WAIT IDLE%% - PROCESSES\n");
    memset(&cpusum, 0, sizeof(cpusum));
    for (i=0; i<n_cpus; i++) {
        cpusum.cpu[CPU_USER] += c_user = cpus[cp][i].cpu[CPU_USER]-cpus[ci][i].cpu[CPU_USER];
        cpusum.cpu[CPU_KERNEL] += c_kern = cpus[cp][i].cpu[CPU_KERNEL]-cpus[ci][i].cpu[CPU_KERNEL];
        cpusum.cpu[CPU_WAIT] += c_wait = cpus[cp][i].cpu[CPU_WAIT]-cpus[ci][i].cpu[CPU_WAIT];
        cpusum.cpu[CPU_IDLE] += c_idle = cpus[cp][i].cpu[CPU_IDLE]-cpus[ci][i].cpu[CPU_IDLE];
#if 0
        cpusum.ctxt  += cpus[cp][i].ctxt-cpus[ci][i].ctxt;
        cpusum.syscall  += cpus[cp][i].syscall-cpus[ci][i].syscall;
        cpusum.sysread  += cpus[cp][i].sysread-cpus[ci][i].sysread;
        cpusum.syswrite += cpus[cp][i].syswrite-cpus[ci][i].syswrite;
        cpusum.writech  += cpus[cp][i].writech-cpus[ci][i].writech;
        cpusum.readch   += cpus[cp][i].readch-cpus[ci][i].readch;
#endif
        c_sum = c_idle+c_user+c_kern+c_wait;
        c_idle = 100.0*c_idle/c_sum;  c_user = 100.0*c_user/c_sum;
        c_kern = 100.0*c_kern/c_sum;  c_wait = 100.0*c_wait/c_sum;
#if 0
        mon_print(6+i,0,"#%-2d %4.0f %4.0f %4.0f %4.0f %4.0f %7.0f %5.0f %5.0f %7.2f %7.2f\n", 
                  i,
                  c_user,c_kern,c_wait,c_idle, 
                  (cpus[cp][i].ctxt-cpus[ci][i].ctxt)/rs,
                  (cpus[cp][i].syscall-cpus[ci][i].syscall)/rs,
                  (cpus[cp][i].syswrite-cpus[ci][i].syswrite)/rs,
                  (cpus[cp][i].sysread-cpus[ci][i].sysread)/rs,
                  (cpus[cp][i].writech-cpus[ci][i].writech)/1024.0/rs,
                  (cpus[cp][i].readch-cpus[ci][i].readch)/1024.0/rs);
#else
        char procslist[128]="";
        for (t=0; t<ntop; t++) {
            if ((top[t].stat == 'R' || top[t].stat == 'D' || top[t].stat == 'W') && top[t].processor == i) {
                strncat(procslist, top[t].progname, sizeof(procslist)-1);
                strncat(procslist, " ", sizeof(procslist)-1);
            }
        }
        mon_print(6+i,0,"#%-2d %4.0f %4.0f %4.0f %4.0f %s\n",
                  i,
                  c_user,c_kern,c_wait,c_idle, procslist);
#endif
    }
    mon_print(6+i,0,"=====================================================================\n");
    c_idle = cpusum.cpu[CPU_IDLE];
    c_user = cpusum.cpu[CPU_USER];
    c_kern = cpusum.cpu[CPU_KERNEL];
    c_wait = cpusum.cpu[CPU_WAIT];
    c_sum = c_idle+c_user+c_kern+c_wait;
    c_idle = 100.0*c_idle/c_sum;  c_user = 100.0*c_user/c_sum;
    c_kern = 100.0*c_kern/c_sum;  c_wait = 100.0*c_wait/c_sum;

#if 0  
    mon_print(6+i+1,0,"SUM %4.0f %4.0f %4.0f %4.0f %4.0f %7.0f %5.0f %5.0f %7.2f %7.2f\n",
              c_user,c_kern,c_wait,c_idle,
              cpusum.ctxt/rs, cpusum.syscall/rs,
              cpusum.syswrite/rs, cpusum.sysread/rs,
              cpusum.writech/1024.0/rs, cpusum.readch/1024.0/rs);
#else
    mon_print(6+i+1,0,"SUM %4.0f %4.0f %4.0f %4.0f \n",
              c_user,c_kern,c_wait,c_idle);
#endif
}

static int summary_dkstat(struct dkstat **sump, struct dkstat *dk1, struct dkstat *dk2, int *active)
{
    int ndisks = 0;
    struct dkstat *sum;
    if (!*sump) {
        *sump = (struct dkstat *) calloc(sizeof(struct dkstat),1);
    }
    sum = *sump;
    memset(sum, 0, sizeof(struct dkstat));
    *active = 0;
    ndisks = 0;
    while (dk1 && dk2) {
        sum->dk_bsize += dk1->dk_bsize;
        sum->dk_rblks += dk1->dk_rblks-dk2->dk_rblks;
        sum->dk_wblks += dk1->dk_wblks-dk2->dk_wblks;
        sum->dk_xfers += dk1->dk_xfers-dk2->dk_xfers;
        sum->dk_time  += dk1->dk_time -dk2->dk_time;
        ndisks++;
        if (dk1->dk_xfers-dk2->dk_xfers) (*active)++;
        dk1 = dk1->dknextp;
        dk2 = dk2->dknextp;
    }
    if (ndisks) {
        sum->dk_bsize /= ndisks;
    }
    return(ndisks);
}
