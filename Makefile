####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/Makefile
# v. 0-1-1:  2001-08-31
#            renovation in support of rpm builds
# v. 0-1-2:  2001-09-05
#            make targets no longer (necessarily) need root 
#            priveleges, and no longer check.  suid of digi 
#            now happens in RPM post install sript
# v. 0-1-3:  2001-09-05
####################################################################

PACKAGE= powerman
VERSION= 0.1.3
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
prefix=	        /usr
exec_prefix=    ${prefix}
bindir=         ${exec_prefix}/bin
sbindir=        ${exec_prefix}/sbin
libdir=         ${exec_prefix}/lib
packagedir=     ${libdir}/${PACKAGE}
# I've removed the doc and packagedoc variables and their install commands.
# I'm pretty sure the the %doc directive in the rpm spec file does that for 
# me.

all: 
	cd src; make; cd ..
	mv src/digi bin
	mv src/ether-wake lib

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: 
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(INSTALL) pm    $(DESTDIR)$(bindir)/
	cd bin; make install; cd ..
	cd etc; make install; cd ..
	cd lib; make install; cd ..
	cd man; make install; cd ..

clean:
	rm -f *~ *.o

allclean: clean
	cd bin; make clean; cd ..
	cd etc; make clean; cd ..
	cd lib; make clean; cd ..
	cd man; make clean; cd ..
	cd src; make clean; cd ..

distclean: clean
	cd bin; make distclean; cd ..
	cd etc; make distclean; cd ..
	cd lib; make distclean; cd ..
	cd man; make distclean; cd ..
	cd src; make distclean; cd ..

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

