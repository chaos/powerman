####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            $POWERMANDIR/Makefile
# v. 0-1-1:  2001-08-31
#            renovation in support of rpm builds
####################################################################

PACKAGE= powerman
VERSION= 0.1.1
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
docdir=         ${prefix}/doc
packagedoc=     ${docdir}/${PACKAGE}-${VERSION}

all: 
	@if test `id -u` != 0 ; then \
	echo "You must be root to do this" ; exit 1 ; fi ; \
	cd src; make; cd ..
	mv src/digi bin
	chown root bin/digi
	chmod 4755 bin/digi
	mv src/ether-wake lib

%:%.c
	$(CC) $(COPTS) $< -o $@ $(INC) $(LIB)

install: 
	@if test `id -u` != 0 ; then \
	echo "You must be root to do this" ; exit 1 ; fi ; \
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(INSTALL) pm    $(DESTDIR)$(bindir)/
	$(mkinstalldirs)             $(DESTDIR)$(packagedoc)
	$(INSTALL) -m 644 DISCLAIMER $(DESTDIR)$(packagedoc)/
	$(INSTALL) -m 644 README     $(DESTDIR)$(packagedoc)/
	$(INSTALL) -m 644 TOUR.SH    $(DESTDIR)$(packagedoc)/
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
	cvs co -r $$tag -p $(PACKAGE)/$(PACKAGE).spec 2>/dev/null ; \
	cp -p $(PACKAGE).spec $$tmp/SPECS/$(PACKAGE).spec; \
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
	cvs export -r $$tag -d $$dir $(PACKAGE) >/dev/null && \
	  (cd $$tmp; tar cf - $$name) | gzip -c9 > $(top_srcdir)/$$name.tgz; \
	rm -rf $$tmp

