####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/src/Makefile
# v. 0-1-1:  2001-08-31
#            renovation in support of rpm builds
####################################################################

CC    = gcc
COPTS = -g -O2 -Wall
INC   = 
LIB   =

all:  ether-wake digi

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

clean:
	rm -f *~ *.o

distclean: clean
	rm -f  ether-wake digi
