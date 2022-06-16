/*
 * Implemention of uptime to aix3.1 with using loadavgd to
 * get real load-values. 
 * Jussi M{ki, Helsinki Univ. of Technology, Computing Centre
 * jmaki@hut.fi, 3.7.1991
 */

#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>
#include <time.h>
#include <utmp.h>
#include <stdio.h>

#define UTMP_FILENAME "/etc/utmp"

main()
{
  int nusers;
  struct tms tbuf;
  struct tm *tm;
  time_t uptime;
  time_t timeofday;
  int days,hours,minutes;
  double loads[3];

  uptime = (times(&tbuf) / HZ);
  timeofday = time((long *)0);
  tm = localtime(&timeofday);

  nusers = getnusers();
  getloadavg(loads,3);

  convsectime(uptime,&days,&hours,&minutes);
  printf(" %2d:%02d%s  up %2d days,  %2d:%02d,  %d users,  load average: %5.2f,%5.2f,%5.2f\n",
	 tm->tm_hour,tm->tm_min,
	 (tm->tm_hour<12)? "am" : "pm",
	 days,hours,minutes,
	 nusers,
	 loads[0],loads[1],loads[2]);
  exit(0);
}

int getnusers()
{
  FILE *utmpf;
  int nusers;
  struct utmp utmp;
  
  nusers=0;
  if (!(utmpf = fopen(UTMP_FILENAME, "r"))) {
    perror(UTMP_FILENAME);
    exit(1);
  }
  while (fread((char *)&utmp,sizeof(utmp),1,utmpf) == 1)
    if (*utmp.ut_name && utmp.ut_type==USER_PROCESS) 
      nusers++;
  fclose(utmpf);
  return (nusers);
}


#define	MINUTES	60
#define HOURS	(MINUTES * 60)
#define DAYS	(HOURS * 24)

convsectime(secs,days,hours,minutes)
time_t secs;
int *days,*hours,*minutes;
{
	secs -= (*days = secs / DAYS) * DAYS;
	secs -= (*hours = secs / HOURS) * HOURS;
	secs -= (*minutes = secs / MINUTES) * MINUTES;
}
