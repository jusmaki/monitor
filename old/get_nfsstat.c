/* monitor -- AIX RS/6000 System monitor 
 *
 * Copyright (c) 1991, 1992, 1993, 1994 Jussi Maki. All Rights Reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE THIS PROGRAM 
 * AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL COPYRIGHTS.
 * Email: jmaki@hut.fi
 */

#include <nlist.h>
#include "get_nfsstat.h"

static struct nlist nfsnames[] = {
    {"rcstat", 0, 0, 0, 0, 0},
    {"rsstat", 0, 0, 0, 0, 0},
    {"clstat", 0, 0, 0, 0, 0},
    {"svstat", 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
};
#define NFS_VALUE(index) (nfsnames[index].n_value)
#define NUMBER_NFS_NLISTS 4
#define NLIST_NFS_RCSTAT 0
#define NLIST_NFS_RSSTAT 1
#define NLIST_NFS_CLSTAT 2
#define NLIST_NFS_SVSTAT 3

int no_nfs = 0;

void get_nfsstat(nfsstat_t **nfsstatp)
{
    static int initted =0;
    nfsstat_t *nfsstat;
    if (! initted) {
      initted = 1;
      if (knlist(nfsnames, NUMBER_NFS_NLISTS, sizeof(struct nlist)) == -1){
	no_nfs = 1;
      }
    }
    if (no_nfs) return;
    if (! *nfsstatp) 
      *nfsstatp = (nfsstat_t *)calloc(sizeof(nfsstat_t),1);
    nfsstat = *nfsstatp;
    getkmemdata(&nfsstat->rc,sizeof(nfs_rcstat_t),NFS_VALUE(NLIST_NFS_RCSTAT));
    getkmemdata(&nfsstat->rs,sizeof(nfs_rsstat_t),NFS_VALUE(NLIST_NFS_RSSTAT));
    getkmemdata(&nfsstat->cl,sizeof(nfs_clstat_t),NFS_VALUE(NLIST_NFS_CLSTAT));
    getkmemdata(&nfsstat->sv,sizeof(nfs_svstat_t),NFS_VALUE(NLIST_NFS_SVSTAT));
}
