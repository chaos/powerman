##*****************************************************************************
## $Id: Makefile.am,v 1.4 2005/07/27 00:03:37 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

DEVFILES = \
	apc7900.dev \
	apc.dev \
	apcnew.dev \
	apcold.dev \
	apcpdu3.dev \
	apcpdu.dev \
	bashfun.dev \
	baytech.dev \
	baytech-rpc28-nc.dev \
	baytech-rpc3-nc.dev \
	cb-7050.dev \
	cyclades-pm10.dev \
	cyclades-pm8.dev \
	dli.dev \
	ibmbladecenter.dev \
	ibmh8.dev \
	icebox.dev \
	icebox3.dev \
	icebox4.dev \
	ilom-inline.dev \
	ipmi.dev \
	ipmipower.dev \
	phantom4.dev \
	phantom.dev \
	powerman.dev \
	swpdu.dev \
	wti.dev \
	wti-rps10.dev

install-data-local:
	-$(top_srcdir)/config/install-sh -m 755 -d \
		$(DESTDIR)$(sysconfdir)/powerman
	for file in $(DEVFILES); do \
		$(top_srcdir)/config/install-sh -m 644 $(srcdir)/$$file $(DESTDIR)$(sysconfdir)/powerman; \
	done

uninstall-local:
	$(RM) -f $(DESTDIR)$(sysconfdir)/powerman

EXTRA_DIST = $(DEVFILES)