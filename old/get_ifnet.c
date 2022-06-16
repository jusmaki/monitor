/* include files needed for ifnet-networkinterface */

#include "getkmemdata.h"
#include "get_ifnet.h"
#include <nlist.h>
#include <fcntl.h>
#include <sys/types.h>

#if defined(AIXv3r1) || defined(AIXv3r2)
#include <sys/comio.h>
#include <sys/devinfo.h>
#include <sys/ciouser.h>
#include <sys/entuser.h>
#endif

#ifndef MAX_NITS
#define MAX_NITS 32
#endif

static struct nlist ifnet_nlist[] = {
    {"ifnet", 0, 0, 0, 0, 0},
    {NULL, 0, 0, 0, 0, 0},
    };

/* get ifnet list structure from kernel and store
 * it to user mode ifnet list 
 * if ifnet_list is NULL then a new list is allocated
 */
int get_ifnet(struct mon_ifnet **ifnet_list)
{
    int if_counter=0;
#ifdef AIX
    static int initted=0;
    static struct ifnet *k_ifnet_list_addr;
    struct ifnet k_ifnet_buf;
    struct ifnet *k_ifnet_addr;
    struct ifnet *ifnet;
    char *if_name;
    struct ifnet *if_next;

    if (!initted) { /* find the address of ifnets structure in k */
	initted=1;
	if (knlist(ifnet_nlist, 1, sizeof(struct nlist)) == -1)
	  perror("knlist, ifnet entry not found");
	getkmemdata((char *)&k_ifnet_list_addr,sizeof(struct ifnet *),
		    (caddr_t)ifnet_nlist[0].n_value);
    }
    k_ifnet_addr = k_ifnet_list_addr;
    if (!*ifnet_list) {
      *ifnet_list = (struct ifnet *)calloc(sizeof(struct ifnet),1);
    }
    ifnet = *ifnet_list;

    /* this code assumes that the order of interfaces don't
     * change in k ifnets list. New interfaces can however be added
     * to the end of list */
    while (k_ifnet_addr) {
      getkmemdata((char *)&k_ifnet_buf, sizeof(struct ifnet), (caddr_t)k_ifnet_addr);
      if_name = ifnet->if_name; 
      if_next = ifnet->if_next;  
      memcpy(ifnet, &k_ifnet_buf, sizeof(struct ifnet));
      ifnet->if_name = if_name; 
      ifnet->if_next = if_next; 
      if (! ifnet->if_name) {
	ifnet->if_name = (char *) calloc(IFNAMSIZ,1);
	getkmemdata(ifnet->if_name, IFNAMSIZ, k_ifnet_buf.if_name);
	/* Check if the information is invalid.             */
	/* This could happen in AIX4 since the last if_next */
	/* pointer is sometimes other that 0x0. Perhaps there is */
        /* another kernel variable telling the number of interfaces defined. */
	/* Have to try to guess when the list is in the end. */
	  
	/* Also according to AIX4 net/netisr.h it seems that the */
	/* 'struct netisr' is the main linked list which should be used */
	/* to collect interface information. */

	if (ifnet->if_name[0] < 32 || ifnet->if_name[0]>127) {
	  break;
	}
      }
#if defined(AIXv3r1) || defined(AIXv3r2)
      /* for AIX3 use COMIO interface to collect fddi statistics */
      if (ifnet->if_name[0] == 'f' && ifnet->if_name[1] == 'i') 
	aix32_get_fddi(ifnet); /* fix the fi0 values by using COMIO to fddi0 */
#endif
      k_ifnet_addr = k_ifnet_buf.if_next;
      if (k_ifnet_addr && !ifnet->if_next) {
	ifnet->if_next = (struct ifnet *) calloc(sizeof(struct ifnet),1);
      }
      ifnet = ifnet->if_next;
      if_counter++;
      /* check the maximun number of interface supported */
      if (if_counter>MAX_NITS) break; 
    }
#endif
    return(if_counter);
}

#if defined(AIXv3r1) || defined(AIXv3r2) 

aix32_get_fddi(struct ifnet *ifnet)
{
  cio_stats_t cio;
  char fddiname[64];
  sprintf(fddiname, "fddi%d",ifnet->if_unit);
  get_cio_stats(fddiname, &cio);
  ifnet->if_ipackets = cio.rx_frame_lcnt;
  ifnet->if_opackets = cio.tx_frame_lcnt;
  ifnet->if_ibytes   = cio.rx_byte_lcnt;
  ifnet->if_obytes   = cio.tx_byte_lcnt;
}

get_cio_stats(char *devicename, cio_stats_t *cio_statsp)
{
  int fd;
  static cio_query_blk_t   query_blk;
  static ent_query_stats_t query_stats;
  cio_stats_t *cio;
  ent_stats_t *ds;

  fd = get_comio_device(devicename);
  query_blk.bufptr = (caddr_t)&query_stats;
  query_blk.buflen = sizeof(query_stats);
  query_blk.clearall = 0;
  cio = &query_stats.cc;
  ds = &query_stats.ds;
  if (ioctl(fd, CIO_QUERY, &query_blk) == -1) {
    perror("ioctl CIO_QUERY to device failed");
    exit(1);
  }
  memcpy(cio_statsp, cio, sizeof(cio_stats_t));
  return(0);
}


#define MAX_COMDEVS 255
int get_comio_device(char *devicename)
{
  static int fds[MAX_COMDEVS];
  static char *fdnames[MAX_COMDEVS];
  static int initted=0;
  char full_dev_name[128];
  int i;
  int fd;
  
  if (!initted) {
    for (i=0; i<MAX_COMDEVS; i++) {
      fds[i]=-1;
      fdnames[i]=(char *)0;
    }
    initted=1;
  }
  
  fd = -1;
  for (i=0; i<MAX_COMDEVS; i++) 
    if (fdnames[i] && strcmp(fdnames[i],devicename)==0) {
      fd = fds[i];
      break;
    } else {
      if (!fdnames[i]) break;
    }
  if (i>=MAX_COMDEVS) {
    printf("get_comio_device(): Error too many communication devices");
    exit(1);
  }
  if (fd == -1) {
    strcpy(full_dev_name, "/dev/");
    strcat(full_dev_name, devicename);
    fd = open(full_dev_name, O_RDONLY);
    if (fd == -1) {
      perror("Opening device");
      printf("name: %s\n", full_dev_name);
      return(-1);
    }
    fdnames[i] = (char *)malloc(strlen(devicename)+1);
    strcpy(fdnames[i], devicename);
    fds[i] = fd;
  }
  return(fd);
}
#endif
