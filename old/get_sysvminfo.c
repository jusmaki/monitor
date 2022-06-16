/* monitor -- AIX RS/6000 System monitor 
 *
 * Copyright (c) 1991 - 1996 Jussi Maki. All Rights Reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU ARE FREE TO DISTRIBUTE THIS PROGRAM 
 * AND MODIFY IT AS LONG AS YOU KEEP ORIGINAL COPYRIGHTS.
 * Email: jmaki@hut.fi
 */

#include <sys/types.h>
#include <nlist.h>
#include "get_sysvminfo.h"
static struct nlist kernelnames[] = {
    {"sysinfo", 0, 0, 0, 0, 0},
    {"vmminfo", 0, 0, 0, 0, 0},
    {"vmker", 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    };
#define N_VALUE(index) (kernelnames[index].n_value)
#define NUMBER_OF_KNSTRUCTS 3
#define NLIST_SYSINFO 0
#define NLIST_VMINFO 1
#define NLIST_VMKER 2
/*
 * Get the system info structure from the running kernel.
 * Get the kernel virtual memory vmker structure
 * Get the kernel virtual memory info structure
 */
void get_sysvminfo(sysvminfo_t **sysvminfo)
{
  static int initted = 0;
  sysvminfo_t *sv;
  if (! initted) {
    initted = 1;
    if (knlist(kernelnames,NUMBER_OF_KNSTRUCTS,sizeof(struct nlist)) == -1){
	perror("knlist, entry not found");
    }
  }
  if (! *sysvminfo) 
    *sysvminfo = (sysvminfo_t *) calloc(sizeof(sysvminfo_t),1);
  sv = *sysvminfo;
  getkmemdata(&sv->sysinfo,sizeof(struct sysinfo),N_VALUE(NLIST_SYSINFO));
  getkmemdata(&sv->vminfo,sizeof(struct vminfo),N_VALUE(NLIST_VMINFO));
  getkmemdata(&sv->vmker,sizeof(struct vmker),N_VALUE(NLIST_VMKER));
  /* adjust memory for 220 models? */
  if (sv->vmker.badmem) sv->vmker.badmem -= 16; 
  sv->vmker.totalmem -= sv->vmker.badmem;
  /* round the amount of memory to the nearest 4 MB, For one J series
     machine the vmker.totalmem did not give a correct number */
  sv->vmker.totalmem = (sv->vmker.totalmem*4096/1024/1024+2)*4/4*1024/4096*1024;
}
