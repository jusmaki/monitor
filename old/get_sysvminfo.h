/* monitor -- AIX RS/6000 System monitor 
 *
 * Copyright (c) 1991, 1992, 1993, 1994 Jussi Maki. All Rights Reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE THIS PROGRAM 
 * AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL COPYRIGHTS.
 * Email: jmaki@hut.fi
 */

#include <sys/sysinfo.h>
#ifdef AIX
#include <sys/vminfo.h>
/* vmker struct is kernelstruct (undocumented) */
/* vmker seems to hold some kernels virtual memeory variables */
#else
struct vminfo {
  int x;
};
#endif


struct vmker {
  uint n0,n1,n2,n3,n4,n5,n6,n7,n8;
  uint totalmem;
  uint badmem; /* this is used in RS/6000 model 220 */
  uint freemem;
  uint n12;
  uint numperm; /* this seems to keep other than text and data segment usage */
                /* the name is taken from /usr/lpp/bos/samples/vmtune.c */
  uint totalvmem,freevmem;
  uint n15, n16, n17, n18, n19;
};
typedef struct sysvminfo {
  struct sysinfo sysinfo;
  struct vminfo vminfo;
  struct vmker vmker;
} sysvminfo_t;

void get_sysvminfo(sysvminfo_t **sysvminfo);
