####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-0-1:  2001-07-11
# v. 0-0-2:  2001-07-31
####################################################################

CC    = gcc
COPTS = -g -O2 -Wall
INC   = 
LIB   =

all:  ether-wake dsr-check dsr-scan

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

clean:
	rm -f *~ *.o ether-wake dsr-check dsr-scan