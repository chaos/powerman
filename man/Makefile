####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/man/Makefile
# v. 0-1-1:  2001-08-31
#            renovation in support of rpm builds
# v. 0-1-2:  2001-09-05
# v. 0-1-3:  2001-09-05
# v. 0-1-4:  2001-09-05
####################################################################

PACKAGE= powerman
VERSION= 0.1.4
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
exec_prefix=    $(prefix)
bindir=         $(exec_prefix)/bin
sbindir=        $(exec_prefix)/sbin
libdir=         $(exec_prefix)/lib
mandir=         $(prefix)/man
packagedir=     $(libdir)/$(PACKAGE)
docdir=         $(prefix)/$(PACKAGE)-$(VERSION)

all: 

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: 
	$(mkinstalldirs)              $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 digi.1      $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 etherwake.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 pm.1        $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 rmc.1       $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 wti.1       $(DESTDIR)$(mandir)/man1
	$(mkinstalldirs)                   $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 digi.conf.5      $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 etherwake.conf.5 $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 powerman.conf.5  $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 wti.conf.5       $(DESTDIR)$(mandir)/man5


clean:
	rm -f *~ *.o

distclean: clean

