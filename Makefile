####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/Makefile
####################################################################

CC    = gcc
COPTS = -g -O2 -Wall
INC   = 
LIB   =

all: 
	cd src; make; cd ..
	mv src/digi bin
	chown root bin/digi
	chmod 4755 bin/digi
	mv src/ether-wake lib

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

clean:
	rm -f *~ *.o

allclean: clean
	cd bin; make clean; cd ..
	cd etc; make clean; cd ..
	cd lib; make clean; cd ..
	cd man; make clean; cd ..
	cd src; make clean; cd ..

distclean: clean
	cd bin; make distclean; cd ..
	cd etc; make distclean; cd ..
	cd lib; make distclean; cd ..
	cd man; make distclean; cd ..
	cd src; make distclean; cd ..

