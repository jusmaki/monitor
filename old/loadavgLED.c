/* loadavgLED.c -- show loadaverage in RS 6000 3 digit LED display
 *
 * Copyright 1994 Jussi M{ki. All Rights reserved.
 * NON-COMMERCIAL USE ALLOWED. YOU MAY USE, COPY AND DISTRIBUTE 
 * THIS PROGRAM FREELY AS LONG AS ORIGINAL COPYRIGHTS ARE LEFT
 * AS THEY ARE.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/mdio.h>

#define MAXPAT 32  /* maximum number of elements in pattern */
#define DELAY  200 /* default delay between display updates */

static int nvram_fd = -1;
#ifdef  ALT_PATTERN
static int pattern[MAXPAT] = {0xa,0xe,0x6,0x8,0x7,0x1,0xb}; /* alt. pattern */
#define NPAT    7
#else
static int pattern[MAXPAT] = {0xa,0xe,0xd,0xc,0xc,0xd,0xe,0xa};/* def.pattern*/
#define NPAT    8
#endif

trap_danger()
{
}

/* in case of signal, clear LED and exit gracefully */
sighandler(int signum)
{
  printf("signal %d occured\n",signum);
  ioctl (nvram_fd, MIONVLED, 0xfff);
  printf("Clearing 3 digit LED display, exiting...\n");
  exit(1);
}

/* test a character string for number-ness */
isnum(char *string)
{
   for(;*string != '\0'; string++)
       if (!isdigit(*string))
      return(0);
   return(1);
}


main(int argc, char *argv[])
{
  double loadavgs[3];
  int c,
      errflg=0,
      delay=DELAY;
  extern char *optarg;
  extern int optind;

  signal(SIGDANGER, (void (*)(int))trap_danger); /* no killing here */
  signal(SIGHUP, (void (*)(int))sighandler);
  signal(SIGINT, (void (*)(int))sighandler);
  signal(SIGTERM, (void (*)(int))sighandler);
  signal(SIGKILL, (void (*)(int))sighandler);
  signal(SIGQUIT, (void (*)(int))sighandler);
  setpriority(PRIO_PROCESS, 0, 20); /* run at lowest priority */

  while ((c=getopt(argc, argv, "d:")) != -1) {
     switch(c) {
        case 'd':
          if (isnum(optarg))
            delay=atoi(optarg);
          else
            errflg++;
          break;
        case '?':
            errflg++;
          break;
      }
      if (errflg) {
         fprintf(stderr, "usage: %s [-d delay (in ms)]\n", argv[0]);
         exit(2);
      }
  }

  /* After AIX 3.2.5 one has to use /dev/nvram instead of /dev/nvram/0 
   */
  if ((nvram_fd = open("/dev/nvram", O_RDONLY))==-1)
    nvram_fd = open("/dev/nvram/0", O_RDONLY);

  delay *= 1000;  /* ms to us */
  while (1) {
    for (c=0; c < NPAT; c++) {
       getloadavg(loadavgs, 3);
       display_loadavg(loadavgs[0], pattern[c]);
       usleep(delay);
    }
  }
}

display_loadavg(double loadavg1, int led_point)
{
  int x;
/*   static int led_point=0xa; rbm */

  if (nvram_fd < 0)
    return;

#ifdef ORIGINAL_LED
  x = (int)(loadavg1*100.0);
#ifdef DEBUG
  fprintf (stderr, "loadavg = %3.5f\n", loadavg1);
  fprintf (stderr, "x = %d\n", x);
#endif
  if (x >= 1000)
    x = 0x999;
  else
    x = (x%10) | (((x/10)%10)<<4) | ((x/100)<<8);
#else
  x = (int)(loadavg1*100.0);

  if (loadavg1<1.0) { x = (led_point<<8) | (((x/10)%10)<<4) | (x%10);    }
  else if (loadavg1<10.0) { x= ((x/100)<<8) | (led_point<<4) | ((x/10)%10); }
  else if (loadavg1<100.0) { x=((x/1000)<<8 | ((x/100)%10)<<4) | (led_point);}
  else x = 0x990|led_point;
/*   led_point = (led_point==0xa)?0xb:0xa; rbm */
#endif
#ifdef DEBUG
  fprintf (stderr, "y = %x\n", x);
#endif
  ioctl (nvram_fd, MIONVLED, x);
}

