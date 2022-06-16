#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

struct mon_ifnet {
  int x;
};
int get_ifnet(struct mon_ifnet **ifnet_list);
