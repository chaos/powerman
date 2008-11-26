##*****************************************************************************
## $Id: Makefile.am,v 1.4 2005/07/27 00:03:37 achu Exp $
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

install-data-local:
	-$(top_srcdir)/config/install-sh -m 755 -d \
		$(DESTDIR)$(sysconfdir)/init.d
	$(top_srcdir)/config/install-sh -m 755 $(srcdir)/powerman.init \
		$(DESTDIR)$(sysconfdir)/init.d/powerman
	-$(top_srcdir)/config/install-sh -m 755 -d \
		$(DESTDIR)$(localstatedir)/run
	-$(top_srcdir)/config/install-sh -o $(RUN_AS_USER) -m 755 -d \
		$(DESTDIR)$(localstatedir)/run/powerman


uninstall-local:
	$(RM) $(DESTDIR)$(sysconfdir)/init.d/powerman

EXTRA_DIST = \
	powerman.init
