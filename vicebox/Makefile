####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
####################################################################
#   Copyright (C) 2001-2002 The Regents of the University of California.
#   Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#   Written by Andrew Uselton (uselton2@llnl.gov>
#   UCRL-CODE-2002-008.
#   
#   This file is part of PowerMan, a remote power management program.
#   For details, see <http://www.llnl.gov/linux/powerman/>.
#   
#   PowerMan is free software; you can redistribute it and/or modify it under
#   the terms of the GNU General Public License as published by the Free
#   Software Foundation; either version 2 of the License, or (at your option)
#   any later version.
#   
#   PowerMan is distributed in the hope that it will be useful, but WITHOUT 
#   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
#   for more details.
#   
#   You should have received a copy of the GNU General Public License along
#   with PowerMan; if not, write to the Free Software Foundation, Inc.,
#   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
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

top_srcdir=     ..
srcbin=		${top_srcdir}/bin
prefix=	        /usr
exec_prefix=    ${prefix}
bindir=         ${exec_prefix}/bin
sbindir=        ${exec_prefix}/sbin
libdir=         ${exec_prefix}/lib
packagedir=     ${libdir}/${PACKAGE}

OBJS = vicebox.o wrappers.o vice_error.o

all :  vicebox svicebox

vicebox : $(OBJS)
	$(CC) $(COPTS) -o ${srcbin}/vicebox $(OBJS) $(INC) $(LIB)

vicebox.o : vicebox.c vicebox.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

wrappers.o : wrappers.c vicebox.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

vice_error.o : vice_error.c vicebox.h
	$(CC) $(COPTS) -c -o $@ $< $(INC)

svicebox : svicebox.c
	$(CC) $(COPTS) -c -o ${srcbin}/svicebox svicebox.c $(INC)

clean :
	rm -f *~ *.o

