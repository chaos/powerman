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
CC=      gcc
DEFS=
CFLAGS=
LDFLAGS=
LIB=
INSTALL= /usr/bin/install -c
mkinstalldirs=	 $(SHELL) $(top_srcdir)/aux/mkinstalldirs
COPTS=   -g -O2 -Wall
INC= 

top_srcdir=     .
prefix=		/usr
exec_prefix=    ${prefix}
bindir=	 ${exec_prefix}/bin
sbindir= ${exec_prefix}/sbin
libdir=	 ${exec_prefix}/lib
mandir=	 $(prefix)/man
etcdir=	 /etc
packagedir=     ${etcdir}/${PACKAGE}
# I've removed the doc and packagedoc variables and their install commands.
# I'm pretty sure the the %doc directive in the rpm spec file does that for 
# me.

all: 
	cd doc; make; cd ..
	cd src; $(MAKE); cd ..
	cd vicebox; make; cd ..

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: 
# All of this needs to be redone
	$(mkinstalldirs)		$(DESTDIR)$(bindir)
	$(INSTALL) bin/pm		$(DESTDIR)$(bindir)/
	$(INSTALL) bin/pmkill		$(DESTDIR)$(bindir)/
	$(mkinstalldirs)		$(DESTDIR)$(packagedir)
	$(INSTALL) bin/digi.py		$(DESTDIR)$(packagedir)
	$(INSTALL) bin/etherwake.py	$(DESTDIR)$(packagedir)
	$(INSTALL) src/ether-wake	$(DESTDIR)$(packagedir)
	$(INSTALL) bin/icebox.py	$(DESTDIR)$(packagedir)
	$(INSTALL) bin/pm_classes.py	$(DESTDIR)$(packagedir)
	$(INSTALL) bin/pm_utils.py	$(DESTDIR)$(packagedir)
	$(INSTALL) bin/rmc		$(DESTDIR)$(packagedir)
	$(INSTALL) bin/rmc.py		$(DESTDIR)$(packagedir)
	$(INSTALL) bin/wti.py		$(DESTDIR)$(packagedir)
	$(mkinstalldirs)		$(DESTDIR)$(etcdir)
	$(INSTALL) -m 644 etc/powerman.conf  $(DESTDIR)$(etcdir)
	$(mkinstalldirs)		$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/pm.1	$(DESTDIR)$(mandir)/man1
	$(mkinstalldirs)		$(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/powerman.conf.5  $(DESTDIR)$(mandir)/man5

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

# DEVELOPER TARGETS
# These need to be replaced with Chris's latest.
rpm tar:
	@if test -z "$(PACKAGE)"; then \
	  echo "ERROR: Undefined PACKAGE macro definition" 1>&2; exit 0; fi; \
	if test -z "$(VERSION)"; then \
	  echo "ERROR: Undefined VERSION macro definition" 1>&2; exit 0; fi; \
	test -z "$$tag" && tag=`echo $(PACKAGE)-$(VERSION) | tr '.' '-'`; \
	$(MAKE) -s $@-internal tag=$$tag ver=$(VERSION)

rpm-internal: tar-internal
	@test -z "$$tag" -o -z "$$ver" && exit 1; \
	tmp=$${TMPDIR-/tmp}/tmp-$(PACKAGE)-$$$$; \
	for d in BUILD RPMS SOURCES SPECS SRPMS TMP; do \
	  $(mkinstalldirs) $$tmp/$$d >/dev/null; \
	done; \
	cp -p $(PACKAGE)-$$ver.tgz $$tmp/SOURCES; \
	cvs -Q co -r $$tag -p $(PACKAGE)/$(PACKAGE).spec \
		 > $$tmp/SPECS/$(PACKAGE).spec; \
	if ! test -s $$tmp/SPECS/$(PACKAGE).spec; then \
	  echo "ERROR: No $(PACKAGE).spec file (tag=$$tag)" 1>&2; \
	  rm -rf $$tmp; exit 0; fi; \
	echo "creating $(PACKAGE)-$$ver*rpm (tag=$$tag)"; \
	rpm --showrc | egrep "_(gpg|pgp)_name" >/dev/null && sign="--sign"; \
	rpm -ba --define "_tmppath $$tmp/TMP" --define "_topdir $$tmp" \
	  $$sign --quiet $$tmp/SPECS/$(PACKAGE).spec && \
	    cp -p $$tmp/RPMS/*/$(PACKAGE)-$$ver*.*.rpm \
	      $$tmp/SRPMS/$(PACKAGE)-$$ver*.src.rpm $(top_srcdir)/; \
	rm -rf $$tmp

tar-internal:
	@test -z "$$tag" -o -z "$$ver" && exit 1; \
	tmp=$${TMPDIR-/tmp}/tmp-$(PACKAGE)-$$$$; \
	name=$(PACKAGE)-$$ver; \
	dir=$$tmp/$$name; \
	echo "creating $$name.tgz (tag=$$tag)"; \
	$(mkinstalldirs) $$tmp >/dev/null; \
	(cd $$tmp; cvs -Q export -r $$tag -d $$name $(PACKAGE) >/dev/null) && \
	  (cd $$tmp; tar cf - $$name) | gzip -c9 > $(top_srcdir)/$$name.tgz; \
	rm -rf $$tmp

