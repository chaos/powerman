####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/lib/Makefile
####################################################################

CC    = gcc
COPTS = -g -O2 -Wall
INC   = 
LIB   =

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

clean:
	rm -f *~ *.o 

distclean: clean
	rm -f ether-wake