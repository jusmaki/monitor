#
# Makefile for loadavgd and monitor -programs
#
# Author: Jussi Maki, jmaki@hut.fi, 31.05.1991
#			modified    06.02.1996
#
# How to install:
#  1. Edit Makefile to contain correct INSTALLDIR  
#     where programs will be installed
#  2. Run make
#  3. Install programs by making monitor and loadavgd suid-programs
#     to root user eg.
#       make install
#

#DEFS=-DUSE_FAST_GETPWUID

INSTALLDIR=/usr/local/bin
VERSION=LINUX

all: 	monitor

monitor:	get_linux.c monitor.c
	cc -g -o monitor monitor.c get_linux.c -lcurses

LOADAVGD_LOCATION=$(INSTALLDIR)/loadavgd

#AIX_VERSION=AIXv"`uname -v`"r"`uname -r`"


clean:
	-/bin/rm -f monitor *.o *~ 
