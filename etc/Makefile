####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/etc/Makefile
# v. 0-1-1:  2001-08-31
#            renovation in support of rpm builds
# v. 0-1-2:  2001-09-05
# v. 0-1-3:  2001-09-05
# v. 0-1-4:  2001-09-05
# v. 0-1-5:  2001-09-05
# v. 0-1-6:  2001-09-07
# v. 0-1-7:  2001-09-17
####################################################################
# Eventually this should build the configuration files from genders,
# dhcpd, or other sources

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
etcdir=         /etc
packagedir=     ${libdir}/${PACKAGE}
docdir=         ${prefix}/$(PACKAGE)-$(VERSION)

all: 

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: 
	$(mkinstalldirs)                 $(DESTDIR)$(etcdir)
	$(INSTALL) -m 644 powerman.conf  $(DESTDIR)$(etcdir)
	$(mkinstalldirs)                 $(DESTDIR)$(packagedir)/etc
	$(INSTALL) -m 644 bogus.conf     $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 digi.conf      $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 etherwake.conf $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 powerman.conf  $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 wti.conf       $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 icebox.conf    $(DESTDIR)$(packagedir)/etc/

clean:
	rm -f *~ *.o

distclean: clean

