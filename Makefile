####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
####################################################################

PACKAGE= powerman
VERSION= 0.1.9
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
packagedir=     ${libdir}/${PACKAGE}
# I've removed the doc and packagedoc variables and their install commands.
# I'm pretty sure the the %doc directive in the rpm spec file does that for 
# me.

all: 
	cd src; $(MAKE); cd ..

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: 
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
	rm -f *~ *.o *.pyc

allclean: clean
	cd bin; rm -f *~ *.o *.pyc; cd ..
	cd etc; rm -f *~ *.o *.pyc; cd ..
	cd man; rm -f *~ *.o *.pyc; cd ..
	cd src; rm -f *~ *.o *.pyc; cd ..

distclean: allclean
	cd src; rm -f ether-wake; cd ..

# DEVELOPER TARGETS

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

