####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/src/Makefile
# v. 0-1-1:  2001-08-31
#            renovation in support of rpm builds
# v. 0-1-2:  2001-09-05
# v. 0-1-3:  2001-09-05
####################################################################

PACKAGE= powerman
VERSION= 0.1.3
SHELL=   /bin/sh
MAKE=    /usr/bin/make
CC    = gcc
DEFS=
CFLAGS=
LDFLAGS=
LIB   =
INSTALL= /usr/bin/install -c
mkinstalldirs=	 $(SHELL) $(top_srcdir)/aux/mkinstalldirs
COPTS = -g -O2 -Wall
INC   = 

top_srcdir=     .
prefix=	        /usr
exec_prefix=    ${prefix}
bindir=         ${exec_prefix}/bin
sbindir=        ${exec_prefix}/sbin
libdir=         ${exec_prefix}/lib
packagedir=     ${libdir}/${PACKAGE}


all:  ether-wake digi

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

clean:
	rm -f *~ *.o

distclean: clean
	rm -f  ether-wake digi
