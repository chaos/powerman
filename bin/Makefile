####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/bin/Makefile
# v. 0-1-1:  2001-08-31
#            renovation in support of rpm builds
# v. 0-1-2:  2001-09-05
# v. 0-1-3:  2001-09-05
# v. 0-1-4:  2001-09-05
# v. 0-1-5:  2001-09-05
# v. 0-1-6:  2001-09-07
# v. 0-1-7:  2001-09-17
####################################################################
# I'll eventually want to exclude "rmc", since it depends on the 
# conman package.  It should be in another rpm by itself.

PACKAGE= powerman
VERSION= 0.1.7
SHELL=   /bin/sh
CC=      gcc
DEFS=
CFLAGS=
LDFLAGS=
LIB=
INSTALL= /usr/bin/install -c
mkinstalldirs=	 $(SHELL) $(top_srcdir)/aux/mkinstalldirs
COPTS=   -g -O2 -Wall
INC= 

top_srcdir=     ..
prefix=	        /usr
exec_prefix=    ${prefix}
bindir=         ${exec_prefix}/bin
sbindir=        ${exec_prefix}/sbin
libdir=         ${exec_prefix}/lib
packagedir=     ${libdir}/${PACKAGE}
docdir=         ${prefix}/$(PACKAGE)-$(VERSION)

all: 

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: 
	$(mkinstalldirs)        $(DESTDIR)$(packagedir)/bin
	$(INSTALL) bogus_check  $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) bogus_set    $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) -m 4755 digi $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) etherwake    $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) rmc          $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) wti          $(DESTDIR)$(packagedir)/bin/

clean:
	rm -f *~ *.o

distclean: clean
	rm -f digi
