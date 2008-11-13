##*****************************************************************************
## $Id: Makefile.am,v 1.4 2005/07/27 00:03:37 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

CONFFILE = \
	powerman.conf.example

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
	cyclades-pm8.dev \
	cyclades-pm10.dev \
	cyclades-pm20.dev \
	cyclades-pm42.dev \
	dli.dev \
	hp3488.dev \
	ibmbladecenter.dev \
	icebox.dev \
	icebox3.dev \
	ics8064.dev \
	ilom.dev \
	ipmi.dev \
	ipmipower.dev \
	phantom.dev \
	plmpower.dev \
	powerman.dev \
	swpdu.dev \
	wti.dev \
	wti-rps10.dev

install-data-local:
	-$(top_srcdir)/config/install-sh -m 755 -d \
		$(DESTDIR)$(sysconfdir)/powerman
	for file in $(DEVFILES) $(CONFFILE); do \
		$(top_srcdir)/config/install-sh -m 644 $(srcdir)/$$file $(DESTDIR)$(sysconfdir)/powerman; \
	done

uninstall-local:
	$(RM) -f $(DESTDIR)$(sysconfdir)/powerman

EXTRA_DIST = $(DEVFILES)
