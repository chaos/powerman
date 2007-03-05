MAKE=    	make
INSTALL= 	/usr/bin/install
mkinstalldirs=	./aux/mkinstalldirs

bindir=		/usr/bin
sbindir=	/usr/sbin
mandir=		/usr/share/man
packagedir=	/etc/powerman
initrddir=	/etc/rc.d/init.d

VERSION = $(shell perl -ne 'print,exit if s/^\s*VERSION:\s*(\S*).*/\1/i' META)

all clean: 
	(cd src && $(MAKE) $@ VERSION=$(VERSION))
	(cd test && $(MAKE) $@)

install: all
	$(mkinstalldirs) 			$(DESTDIR)$(bindir)
	$(INSTALL) src/powerman			$(DESTDIR)$(bindir)
	ln -s powerman				$(DESTDIR)$(bindir)/pm
	$(mkinstalldirs) 			$(DESTDIR)$(sbindir)
	$(INSTALL) src/powermand		$(DESTDIR)$(sbindir)
	$(mkinstalldirs)			$(DESTDIR)$(packagedir)
	$(INSTALL) -m 644 etc/*.dev		$(DESTDIR)$(packagedir)
	$(mkinstalldirs)			$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/powerman.1	$(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 man/powermand.1	$(DESTDIR)$(mandir)/man1
	$(mkinstalldirs) 			$(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/powerman.conf.5  	$(DESTDIR)$(mandir)/man5
	$(INSTALL) -m 644 man/powerman.dev.5  	$(DESTDIR)$(mandir)/man5
	$(mkinstalldirs) 			$(DESTDIR)$(mandir)/man7
	$(INSTALL) -m 644 man/powerman-devices.7 $(DESTDIR)$(mandir)/man7
	$(mkinstalldirs) 			$(DESTDIR)$(initrddir)
	$(INSTALL) -m 755 scripts/powerman.init $(DESTDIR)$(initrddir)/powerman

distclean: clean
	rm -f *.rpm *.bz2

testrpm:
	scripts/build --snapshot .
