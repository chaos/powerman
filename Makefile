####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
####################################################################

PACKAGE= powerman
VERSION= 0.1.7
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
sbindir=	${exec_prefix}/sbin
libdir=	 ${exec_prefix}/lib
mandir=	 $(prefix)/man
etcdir=	 /etc
packagedir=     ${libdir}/${PACKAGE}
docdir=	 ${prefix}/$(PACKAGE)-$(VERSION)
# I've removed the doc and packagedoc variables and their install commands.
# I'm pretty sure the the %doc directive in the rpm spec file does that for 
# me.

all: 
	cd src; $(MAKE); cd ..

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: 
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(INSTALL) pm    $(DESTDIR)$(bindir)/
	$(mkinstalldirs)	   $(DESTDIR)$(packagedir)/bin
	$(INSTALL) bin/bogus_check $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) bin/bogus_set   $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) bin/digi	$(DESTDIR)$(packagedir)/bin/
	$(INSTALL) bin/etherwake   $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) bin/icebox      $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) bin/rmc	 $(DESTDIR)$(packagedir)/bin/
	$(INSTALL) bin/wti	 $(DESTDIR)$(packagedir)/bin/
	$(mkinstalldirs)		 $(DESTDIR)$(etcdir)
	$(INSTALL) -m 644 etc/powerman.conf  $(DESTDIR)$(etcdir)
	$(mkinstalldirs)		 $(DESTDIR)$(packagedir)/etc
	$(INSTALL) -m 644 etc/bogus.conf     $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 etc/digi.conf      $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 etc/etherwake.conf $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 etc/powerman.conf  $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 etc/wti.conf       $(DESTDIR)$(packagedir)/etc/
	$(INSTALL) -m 644 etc/icebox.conf    $(DESTDIR)$(packagedir)/etc/
	$(mkinstalldirs)      $(DESTDIR)$(packagedir)/lib
	$(INSTALL) src/ether-wake $(DESTDIR)$(packagedir)/lib/
	$(mkinstalldirs)	      $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/digi.1      $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/etherwake.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/icebox.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/pm.1	$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/rmc.1       $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/wti.1       $(DESTDIR)$(mandir)/man1
	$(mkinstalldirs)		   $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/digi.conf.5      $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/etherwake.conf.5 $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/icebox.conf.5 $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/powerman.conf.5  $(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/wti.conf.5       $(DESTDIR)$(mandir)/man5

clean:
	rm -f *~ *.o

allclean: clean
	cd bin; rm -f *~ *.o; cd ..
	cd etc; rm -f *~ *.o; cd ..
	cd lib; rm -f *~ *.o; cd ..
	cd man; rm -f *~ *.o; cd ..
	cd src; rm -f *~ *.o; cd ..

distclean: allclean
	cd lib; rm -f ether-wake; cd ..
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

