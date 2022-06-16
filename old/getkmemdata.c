/*
 * (C) Copyright 1994, Jussi Maki (Email: jmaki@hut.fi). All rights reserved
 * This code is part of AIX monitor program. 
 * Permission to copy and use this source for noncommersial use.
 * For commercial use a permission must be asked from author.
 */

#include <unistd.h>
#include <fcntl.h>
#include "getkmemdata.h"

/*********************************************************************/
int getkmemdata(char *buf,int bufsize,caddr_t address)
{
    static int fd;
    static initted = 0;
    int n;
    /*
    ** Do stuff we only need to do once per invocation, like opening
    ** the kmem file and fetching the parts of the symbol table.
    */
    if (!initted) {
	initted = 1;
	fd = open("/dev/kmem", O_RDONLY);
	if (fd < 0) {
	    perror("kmem");
	    exit(1);
	}
    }
    /*
    ** Get the structure from the running kernel.
    */
    lseek(fd, (off_t) address, SEEK_SET);
    n = read(fd, buf, bufsize);
    return(n);
}
