##*****************************************************************************
## $Id: Makefile.am,v 1.4 2005/07/27 00:03:37 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

install-data-local:
	$(top_srcdir)/config/install-sh -m 755 -d $(DESTDIR)/$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/apc.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/apc7900.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/apcnew.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/apcpdu.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/baytech.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/baytech-rpc28-nc.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/baytech-rpc3-nc.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/cb-7050.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/cyclades-pm8.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/cyclades-pm10.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/ibmbladecenter.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/ibmh8.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/icebox.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/icebox3.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/icebox4.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/ipmi.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/ipmipower.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/phantom.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/phantom4.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/powerman.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/vpc.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/wti.dev $(DESTDIR)$(sysconfdir)/powerman
	$(top_srcdir)/config/install-sh -m 644 $(srcdir)/wti-rps10.dev $(DESTDIR)$(sysconfdir)/powerman

uninstall-local:
	$(RM) -f $(DESTDIR)$(sysconfdir)/powerman

EXTRA_DIST = \
	apc.dev \
	apc7900.dev \
	apcnew.dev \
	apcpdu.dev \
	baytech.dev \
	baytech-rpc28-nc.dev \
	baytech-rpc3-nc.dev \
	cb-7050.dev \
	cyclades-pm8.dev \
	cyclades-pm10.dev \
	ibmbladecenter.dev \
	ibmh8.dev \
	icebox.dev \
	icebox3.dev \
	icebox4.dev \
	ipmi.dev \
	ipmipower.dev \
	phantom.dev \
	phantom4.dev \
	powerman.dev \
	vpc.dev \
	wti.dev \
	wti-rps10.dev
