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

NAME= $(shell perl -ne 'print,exit if s/^\s*NAME:\s*(\S*).*/\1/i' META)
VERSION = $(shell perl -ne 'print,exit if s/^\s*VERSION:\s*(\S*).*/\1/i' META)
RELEASE = $(shell perl -ne 'print,exit if s/^\s*RELEASE:\s*(\S*).*/\1/i' META)
SHELL=   /bin/sh
MAKE=    /usr/bin/make
CC=      gcc
DEFS=
CFLAGS=
LDFLAGS=
LIB=
INSTALL= /usr/bin/install -c
mkinstalldirs=	 $(SHELL) $(top_srcdir)/aux/mkinstalldirs
COPTS=   -g -O2 -Wall
INC= 

top_srcdir	=	.
prefix		=	/usr
exec_prefix	=	${prefix}
bindir		=	${exec_prefix}/bin
sbindir		= 	${exec_prefix}/sbin
libdir		=	${exec_prefix}/lib
mandir		=	$(prefix)/man
etcdir		=	/etc
packagedir	=	${etcdir}/${NAME}
piddir		=	/var/run/${NAME}
docdir		=	${prefix}/share/doc
# I've removed the doc and packagedoc variables and their install commands.
# I'm pretty sure the the %doc directive in the rpm spec file does that for 
# me.

all: progs tests

progs : 
	$(MAKE) -C src NAME=$(NAME) VERSION=$(VERSION) RELEASE=$(RELEASE)

tests : 
	$(MAKE) -C test

docs : 
	$(MAKE) -C doc 

.c.o:
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: all
	$(mkinstalldirs)			$(DESTDIR)$(bindir)
	$(INSTALL) src/powerman			$(DESTDIR)$(bindir)/
	ln -s $(bindir)/powerman		$(DESTDIR)$(bindir)/off
	ln -s $(bindir)/powerman		$(DESTDIR)$(bindir)/on
	ln -s $(bindir)/powerman		$(DESTDIR)$(bindir)/pm
	$(mkinstalldirs)			$(DESTDIR)$(sbindir)
	$(INSTALL) src/powermand		$(DESTDIR)$(sbindir)/
	$(mkinstalldirs)			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/baytech.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/icebox.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/icebox3.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/wti.dev			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/vicebox.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/vpc.dev			$(DESTDIR)$(packagedir)
	$(mkinstalldirs)			$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/powerman.1	$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/powermand.1	$(DESTDIR)$(mandir)/man1
	$(mkinstalldirs)			$(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/powerman.conf.5  	$(DESTDIR)$(mandir)/man5
	$(mkinstalldirs)			$(DESTDIR)$(piddir)
	$(mkinstalldirs)			$(DESTDIR)$(docdir)
	if ! test -f $(DESTDIR)/etc/rc.d/init.d/powerman; then \
	  $(mkinstalldirs) $(DESTDIR)/etc/rc.d/init.d; \
	  $(INSTALL) -m 755 scripts/powerman.init \
	  $(DESTDIR)/etc/rc.d/init.d/powerman; \
	fi


uninstall: distclean
	rm -f $(DESTDIR)$(bindir)/powerman
	rm -f $(DESTDIR)$(sbindir)/powermand
	rm -f $(DESTDIR)$(packagedir)/baytech.dev
	rm -f $(DESTDIR)$(packagedir)/icebox.dev
	rm -f $(DESTDIR)$(packagedir)/pmd.dev
	rm -f $(DESTDIR)$(packagedir)/wti.dev
	rm -f $(DESTDIR)$(packagedir)/vicebox.dev
	rm -f $(DESTDIR)$(packagedir)/vpc.dev
	rm -f $(DESTDIR)$(mandir)/man1/powerman.1
	rm -f $(DESTDIR)$(mandir)/man1/powermand.1
	rm -f $(DESTDIR)$(mandir)/man5/powerman.conf.5
	rm -Rf $(DESTDIR)$(piddir)
	rm -Rf $(DESTDIR)$(docdir)

clean:
	find . -name \*~ -o -name .#* | xargs rm -f
	make -C src clean
	make -C doc clean
	make -C test clean

distclean: clean
	rm -f *.tgz* *.rpm

include Make-rpm.mk
