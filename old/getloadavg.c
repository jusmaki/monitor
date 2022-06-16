/*
 * getloadavg.c -- routine to get loadavgd-values
 * this version is used with loadavgd-server in IBM AIX3.
 * Note, that in AIX 3.2.4 IBM included avenrun structure in
 * aix kernel which can be used instead of loadavgd daemon.
 *
 * $Id: getloadavg.c,v 1.3 1993/10/22 19:26:46 jmaki Exp $
 *
 */

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <nlist.h>
#include <fcntl.h>

#include "getloadavg.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct { long l1, l5, l15; } avenrun_t;

int signal_handler(sig, code, scp)
int sig;
int code;
struct sigcontext *scp;
{
    /* set this signal-handler again */
    signal(sig,(void (*)(int))signal_handler);
}

getloadavg(loadv,nelem)
double loadv[];
int nelem;
{
    static int initted=0;
    static int sock;
    static struct sockaddr_in server;
    static struct hostent *hp;
    static void ((*alarm_sig)(int));
    char buf[80];
    int status;
    int server_len = sizeof(struct sockaddr_in);
    int errno2;
    int i;

    if (getloadavg_avenrun(loadv, nelem) != -1) {
    } else {
      if (! initted) {
	initted=1;
        sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) { perror("opening datagram socket"); exit(1); }
	server.sin_family = AF_INET;
	hp = gethostbyname("localhost");
	if (hp == 0) { fprintf(stderr,"localhost unknown\n"); exit(1); }
	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_port = htons(LOADAVGD_PORT);
      }
    
      /* send something to loadavgd-server to get reply back */
      status = sendto(sock,"\n\000",2,0, &server,sizeof(server));
      if (status == -1) {perror("getloadavg: sendto loadavgd");exit(1);}
      
      alarm_sig = signal(SIGALRM,(void (*)(int))signal_handler);
      alarm(2); /* lets wait for 2 second to get respond from server */
      status = recvfrom(sock, buf, 80,0, &server, &server_len);
      errno2 = errno;
      alarm(0);
      signal(SIGALRM,alarm_sig);

      if (status==-1) {
	if (errno2 == EINTR) { /* if recvfrom was interrupter by alarm */
	  fprintf(stderr,"\n***getloadavg: starting loadavgd-daemon ***\n");
	  launch_loadavgd();
	  fflush(stderr);
	  sleep(1);
	  status = sendto(sock,"\n\000",2,0, &server,sizeof(server));
	  if (status == -1) {perror("getloadavg: sendto loadavgd");exit(1);}
	  status = recvfrom(sock, buf, 80,0, &server, &server_len);
	}
	if (status == -1) {
	    perror("getloadavg: recvfrom from loadavgd daemon");
	    exit(1);
	}
      }
      if (nelem == 3) sscanf(buf,"%lf %lf %lf",&loadv[0],&loadv[1],&loadv[2]);
      if (nelem == 2) sscanf(buf,"%lf %lf",&loadv[0],&loadv[1]);
      if (nelem == 1) sscanf(buf,"%lf",&loadv[0]);
    }
}


getloadavg_avenrun(double loadv[], int nelem)
{
  static int initted=0;
  int avenrun[3];
  static struct nlist kernelnames[] = {
    {"avenrun", 0, 0, 0, 0, 0},
    {NULL, 0, 0, 0, 0, 0}
  };
  static int no_avenrun_here = 0;

  if (no_avenrun_here) 
    return -1;

  if (!initted) {
    initted = 1;
    if (knlist(kernelnames,1,sizeof(struct nlist)) == -1){
      no_avenrun_here = 1;
      return -1;
    }
  }
  getkmemdata(&avenrun, sizeof(avenrun), kernelnames->n_value);
  if (nelem > 0) loadv[0]=avenrun[0]/65536.0;
  if (nelem > 1) loadv[1]=avenrun[1]/65536.0;
  if (nelem > 2) loadv[2]=avenrun[2]/65536.0;
  return(0);
}


