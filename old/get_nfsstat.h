/* monitor -- AIX RS/6000 System monitor 
 *
 * Copyright (c) 1991, 1992, 1993, 1994 Jussi Maki. All Rights Reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE THIS PROGRAM 
 * AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL COPYRIGHTS.
 * Email: jmaki@hut.fi
 */
#include <sys/types.h>
/* 
 * Following nfs kernel statistics structures are uncodumented.
 * The approximate content of the structure was solved using 
 * the output and the manual page of the /usr/etc/nfsstat command.
 */
typedef struct { /* Client rpc: */
  uint calls, badcalls;
  uint retrans;
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
  uint calls;
  uint nclget;
  uint badcalls;
  uint null;
  uint getattr;
  uint setattr;
  uint root;
  uint lookup;
  uint readlink;
  uint read;
  uint wrcache;
  uint write;
  uint create;
  uint remove;
  uint rename;
  uint link;
  uint symlink;
  uint mkdir;
  uint rmdir;
  uint readdir;
  uint fsstat;
} nfs_clstat_t;

typedef struct {/* Server nfs: */
  uint calls;
  uint badcalls;
  uint null;
  uint getattr;
  uint setattr;
  uint root;
  uint lookup;
  uint readlink;
  uint read;
  uint wrcache, write;
  uint create, remove, rename;
  uint link, symlink;
  uint mkdir, rmdir,readdir;
  uint fsstat;
} nfs_svstat_t;

typedef struct {
  nfs_rcstat_t rc;
  nfs_rsstat_t rs;
  nfs_clstat_t cl;
  nfs_svstat_t sv;
} nfsstat_t;

void get_nfsstat(nfsstat_t **nfsstatp);
extern int no_nfs; /* true if nfs is not available */
