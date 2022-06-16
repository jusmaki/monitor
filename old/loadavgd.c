/* loadavgd.c -- load average daemon for AIX 3.1
** Copyright 1993 Jussi M{ki. All Rights reserved.
** NON-COMMERCIAL USE ALLOWED. YOU MAY USE, COPY AND DISTRIBUTE 
** THIS PROGRAM FREELY AS LONG AS ORIGINAL COPYRIGHTS ARE LEFT
** AS THEY ARE.
**
** History:
** Created 27.5.1991 as part of monitor v1.02 program
** first version made as tcp-service. Second version changed to 
** use datagram (udp) service.
**
** Use getloadavg()-subroutine (getloadavg.c) to get 
** currrent loadavg-values. 
*/

#include <sys/types.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <sys/nlist.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <locale.h>
/*
#define DEBUG 1
*/

#define MIN(a,b) ((a)<(b)?(a):(b))

int master_sd;
int master_port;


main(int argc, char *argv[])
{
    int tty;
    setlocale(LC_NUMERIC,"C"); /* be sure that decimal-point is "." */
    if (argc<2) {
	fprintf(stderr,"Usage: loadavgd portnumber\n");
	exit(1);
    }
    master_port=atoi(argv[1]);
    if (master_port<1) {
	fprintf(stderr,"Usage: loadavgd portnumber\n");
	exit(1);
    }
#ifndef DEBUG
    close(0);close(1);close(2);
    /* release this process from tty-control */
    setsid();
    if ((tty = open ("/dev/tty", 2)) >= 0) {
      ioctl (tty, TIOCNOTTY, (char *) NULL);
      close (tty);
    }
#endif

    init_signals();
    init_socket();
    handle_loadavg();
    while (1) {
	handle_io();
	handle_loadavg();
    }
}

void signal_handler(int sig, int code, struct sigcontext *scp)
{
#ifdef DEBUG
    fprintf(stderr,"Signal %d occured\n",sig);
    if (sig == SIGINT) exit(1);
    fflush(stderr);
#endif
    /* set this signal-handler again */
    (void)signal(sig, (void (*)(int))signal_handler);
    if (sig == SIGALRM)
      alarm (5);
}

init_signals()
{
  int i;

  for (i=1; i<32; i++) {
      signal(i, (void (*)(int))signal_handler);
  }
  signal(SIGALRM,(void (*)(int))signal_handler);
  alarm(5);
}

init_socket()
{
    int i,namesize;
    int status;
    struct sockaddr_in name_master;
    int port_master;

    master_sd = socket(AF_INET,SOCK_DGRAM,0);
    if (master_sd == -1) { perror("opening socket"); exit(1); }

    name_master.sin_family = AF_INET;
    name_master.sin_addr.s_addr = INADDR_ANY;
    name_master.sin_port = master_port;

    status = bind(master_sd,&name_master,sizeof(name_master));
    if (status) { perror("binding socket"); exit(1); }
}

handle_io()
{
    int status;
    char input_buf[BUFSIZ];
    static char output_buf[BUFSIZ];
    double l1,l5,l15;
    struct sockaddr from_addr;
    int from_len = sizeof(struct sockaddr);

    status = recvfrom(master_sd, input_buf, BUFSIZ,0,
		      &from_addr, &from_len);
    if (status != -1) {
	calcloadavg(&l1,&l5,&l15);
	sprintf(output_buf,"%f %f %f\n",l1,l5,l15);
#ifdef DEBUG
	printf("loadavgd handle_io: %f %f %f\n",l1,l5,l15);
#endif
	status = sendto(master_sd,output_buf,strlen(output_buf)+1,0,
			&from_addr,from_len);
    }
}

handle_loadavg()
{
    static int initted=0;
    static int fd;
    static struct sysinfo si;
    static struct nlist kernelnames[] = {
	{"sysinfo", 0, 0, 0, 0, 0},
	{NULL, 0, 0, 0, 0, 0},
        };

    if (!initted) {
	initted = 1;
	fd = open("/dev/kmem", O_RDONLY);
	if (fd < 0) {
	    perror("kmem");
	    exit(1);
	}
	if (knlist(kernelnames,1,sizeof(struct nlist)) == -1) {
	    perror("knlist, entry not found");
	}
    }
    lseek(fd, kernelnames[0].n_value,SEEK_SET);
    read(fd,&si,sizeof(si));
#ifdef DEBUG
    printf("runque, runocc %8d %8d\n",si.runque,si.runocc);
#endif
    update_loadavg(si.runque,si.runocc);
}

