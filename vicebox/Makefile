####################################################################
# $Id$
# vicebox - The Virtual IceBox
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2001 Regents of the University of California
# See ./DISCLAIMER
# v. 0-1-0:  2002-02-27
#            Originally developed in conjunction with the PowerMan
#            project and moved to a separate project on this date.
####################################################################

PACKAGE= powerman
VERSION= 1.0.0
SHELL=   /bin/sh
MAKE=    /usr/bin/make
CC    = gcc
LEX   = /usr/bin/flex
YACC  = /usr/bin/bison
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

OBJS = vicebox.o wrappers.o vice_error.o

all :  vicebox svicebox

vicebox : $(OBJS)
	$(CC) $(COPTS) -o vicebox $(OBJS) $(INC) $(LIB)

vicebox.o : vicebox.c vicebox.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

wrappers.o : wrappers.c vicebox.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

vice_error.o : vice_error.c vicebox.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

svicebox : svicebox.c
	$(CC) $(COPTS) -c -o $@ $< $(INC)

clean :
	rm -f *~ *.o

distclean : clean
	rm -f  vicebox svicebox
