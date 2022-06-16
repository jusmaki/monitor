/* launch_loadavgd.c -- start loadavgd process 
** Called from getloadavg if loadavgd server is not running
** or accepting connections to port.
**
*/

#include <stdio.h>
#include "getloadavg.h"

launch_loadavgd()
{
    char buf[32];
    int i;
    
    sprintf(buf,"%d",LOADAVGD_PORT);
    switch (fork()) {
      case 0: /* child */
	execl(LOADAVGD_LOCATION,"loadavgd",buf,0);
	perror("cannot exec");
	fprintf(stderr,"cannot start loadavgd-daemon %s\n",LOADAVGD_LOCATION);
	exit(2);
	/* never returns */
      case -1: /* error */
	perror("cannot fork loadavgd");
	exit(2);
      default: /*parent */
	break;
    }
}
