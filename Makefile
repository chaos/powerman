PROJECT = powerman
VERSION = $(shell perl -ne 'print,exit if s/^\s*VERSION:\s*(\S*).*/\1/i' META)
SHELL=   /bin/sh
MAKE=    /usr/bin/make
CC=      gcc
INSTALL= /usr/bin/install -c
mkinstalldirs=	 $(SHELL) $(top_srcdir)/aux/mkinstalldirs
CFLAGS=   -g -O2 -Wall

top_srcdir	=	.
prefix		=	/usr
exec_prefix	=	${prefix}
bindir		=	${exec_prefix}/bin
sbindir		= 	${exec_prefix}/sbin
mandir		=	$(prefix)/man
etcdir		=	/etc
packagedir	=	${etcdir}/${PROJECT}

all: progs tests

progs : 
	$(MAKE) -C src VERSION=$(VERSION)

tests : 
	$(MAKE) -C test

install: all
	$(mkinstalldirs) 			$(DESTDIR)$(bindir)
	$(INSTALL) src/powerman			$(DESTDIR)$(bindir)/
	ln -s $(bindir)/powerman		$(DESTDIR)$(bindir)/pm
	$(mkinstalldirs) 			$(DESTDIR)$(sbindir)
	$(INSTALL) src/powermand		$(DESTDIR)$(sbindir)/
	$(mkinstalldirs)			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/baytech.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/baytech-rpc28-nc.dev	$(DESTDIR)$(packagedir)
	$(INSTALL) etc/baytech-rpc3-nc.dev	$(DESTDIR)$(packagedir)
	$(INSTALL) etc/icebox.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/icebox3.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/wti.dev			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/wti-rps10.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/apc.dev			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/apcnew.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/apcpdu.dev		$(DESTDIR)$(packagedir)
#	$(INSTALL) etc/vicebox.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/vpc.dev			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/ibmh8.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/phantom.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/phantom4.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/ipmi.dev			$(DESTDIR)$(packagedir)
	$(INSTALL) etc/cyclades-pm8.dev		$(DESTDIR)$(packagedir)
	$(INSTALL) etc/cyclades-pm10.dev	$(DESTDIR)$(packagedir)
	$(INSTALL) etc/ipmipower.dev            $(DESTDIR)$(packagedir)
	$(INSTALL) etc/ibmbladecenter.dev       $(DESTDIR)$(packagedir)
	$(INSTALL) etc/cb-7050.dev              $(DESTDIR)$(packagedir)
	$(mkinstalldirs)			$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/powerman.1	$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/powermand.1	$(DESTDIR)$(mandir)/man1
	$(mkinstalldirs) 			$(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/powerman.conf.5  	$(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/powerman.dev.5  	$(DESTDIR)$(mandir)/man5
	$(mkinstalldirs) 			$(DESTDIR)$(mandir)/man7
	$(INSTALL) -m 644 man/powerman-devices.7 $(DESTDIR)$(mandir)/man7
	$(mkinstalldirs) 			$(DESTDIR)/etc/rc.d/init.d
	$(INSTALL) -m 755 scripts/powerman.init $(DESTDIR)/etc/rc.d/init.d/powerman

clean:
	make -C src clean
	make -C test clean
	find . -name \*~ -o -name .\#* | xargs rm -f

distclean: clean
	rm -f *.tgz* *.rpm

