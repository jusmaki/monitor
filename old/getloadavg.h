
extern getloadavg(double loadv[], int nelem);

#ifndef LOADAVGD_PORT
#define LOADAVGD_PORT 2112
#endif
#ifndef LOADAVGD_LOCATION
#define LOADAVGD_LOCATION "/usr/local/bin/loadavgd"
#endif
