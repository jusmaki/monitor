/* monitor -- AIX RS/6000 System monitor 
 *
 * Copyright (c) 1991, 1992, 1993, 1994 Jussi Maki. All Rights Reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE THIS PROGRAM 
 * AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL COPYRIGHTS.
 * Email: jmaki@hut.fi
 */
#include <nlist.h>
#include <unistd.h>
#include "get_dkstat.h"
#include "getkmemdata.h"

static struct nlist iostat_nlist[] = {
    {"iostat", 0, 0, 0, 0, 0},
    {NULL, 0, 0, 0, 0, 0},
    };


/* get the dkstat list from kernel. This subroutine will allocate 
 * required memory if list is too short or empty 
 * Usage: struct dkstat *dkstat; int dk_cnt;
 *        dk_cnt = get_dkstat(&dkstat);
 */
int get_dkstat(struct dkstat **dkstat_list)
{
    static int initted=0;
#ifdef AIX
    static struct iostat *k_iostat_addr;
    struct iostat k_iostat_buf;
    struct dkstat *dk, *u_dknextp, *k_dknextp;
    int i;

    if (!initted) {
	initted = 1;
	if (knlist(iostat_nlist, 1, sizeof(struct nlist)) == -1)
	  perror("knlist, iostat entry not found");
	k_iostat_addr = (struct iostat *)iostat_nlist[0].n_value;
    }
    if (!*dkstat_list) {
      *dkstat_list = (struct dkstat *)calloc(sizeof(struct dkstat),1);
    }
    dk = *dkstat_list;
    
    getkmemdata((char *)&k_iostat_buf,sizeof(struct iostat), (caddr_t)k_iostat_addr);
    k_dknextp = k_iostat_buf.dkstatp;
    u_dknextp = dk->dknextp;
    getkmemdata((char *)dk, sizeof(struct dkstat), (caddr_t)k_dknextp);
    k_dknextp = dk->dknextp;
    dk->dknextp = u_dknextp;
    for (i=1; i<k_iostat_buf.dk_cnt; i++) {
      if (! dk->dknextp) 
	dk->dknextp = (struct dkstat *)calloc(sizeof(struct dkstat),1);
      dk = dk->dknextp;
      u_dknextp = dk->dknextp;
      getkmemdata((char *)dk, sizeof(struct dkstat), (caddr_t)k_dknextp);
      k_dknextp = dk->dknextp;
      dk->dknextp = u_dknextp;
    }
    return(k_iostat_buf.dk_cnt);
#else
    return 0;
#endif
}

int summary_dkstat(struct dkstat **sump, struct dkstat *dk1, struct dkstat *dk2, int *active)
{
  int ndisks = 0;
  struct dkstat *sum;
#ifdef AIX
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
#endif
  return(ndisks);
}
