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

PROJECT= powerman
PACKAGE= powerman
VERSION= 1.0.0
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
packagedir	=	${etcdir}/${PACKAGE}
piddir		=	/var/run/${PACKAGE}
docdir		=	${prefix}/share/doc
# I've removed the doc and packagedoc variables and their install commands.
# I'm pretty sure the the %doc directive in the rpm spec file does that for 
# me.

all: progs docs

progs : 
	cd src; $(MAKE); cd ..
	cd vicebox; $(MAKE); cd ..

docs : 
	cd doc; $(MAKE); cd ..

.c.o:
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: all
	$(mkinstalldirs)			$(DESTDIR)$(bindir)
	$(INSTALL) bin/powerman			$(DESTDIR)$(bindir)/
	$(INSTALL) bin/powermand		$(DESTDIR)$(bindir)/
	$(mkinstalldirs)			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/baytech.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/icebox.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/pmd.dev			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/wti.dev			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/vicebox.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) -m 644 etc/powerman.conf	$(DESTDIR)$(packagedir)
	$(mkinstalldirs)			$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/powerman.1	$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/powermand.1	$(DESTDIR)$(mandir)/man1
	$(mkinstalldirs)			$(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/powerman.conf.5  	$(DESTDIR)$(mandir)/man5
	$(mkinstalldirs)			$(DESTDIR)$(piddir)
	$(mkinstalldirs)			$(DESTDIR)$(docdir)

uninstall: distclean
	rm -f $(DESTDIR)$(bindir)/powerman
	rm -f $(DESTDIR)$(bindir)/powermand
	rm -f $(DESTDIR)$(packagedir)/baytech.dev
	rm -f $(DESTDIR)$(packagedir)/icebox.dev
	rm -f $(DESTDIR)$(packagedir)/pmd.dev
	rm -f $(DESTDIR)$(packagedir)/wti.dev
	rm -f $(DESTDIR)$(packagedir)/vicebox.dev
	mv $(DESTDIR)$(packagedir)/powerman.conf	$(packagedir)/powerman.conf.bak
	rm -f $(DESTDIR)$(mandir)/man1/powerman.1
	rm -f $(DESTDIR)$(mandir)/man1/powermand.1
	rm -f $(DESTDIR)$(mandir)/man5/powerman.conf.5
	rm -Rf $(DESTDIR)$(piddir)
	rm -Rf $(DESTDIR)$(docdir)

clean:
	rm -f *~ *.o .#*

allclean: clean
	cd aux; rm -f *~ *.o .#*; cd ..
	cd bin; rm -f *~ *.o .#*; cd ..
	cd doc; make clean; cd ..
	cd etc; rm -f *~ *.o .#*; cd ..
	cd lib; rm -f *~ *.o .#*; cd ..
	cd log; rm -f *~ *.o .#*; cd ..
	cd man; rm -f *~ *.o .#*; cd ..
	cd scripts; rm -f *~ *.o .#*; cd ..
	cd src; make clean; cd ..
	cd vicebox; make clean; cd ..

distclean: allclean
	cd bin; rm -f powerman powermand vicebox svicebox; cd ..
	cd doc; rm -f *.ps; cd ..
	rm -f *.tgz* *.rpm

include Make-rpm.mk
