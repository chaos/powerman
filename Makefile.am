SUBDIRS = \
	etc \
	man \
	scripts \
	heartbeat \
	liblsd \
	libcommon \
	powerman \
	powermand \
	libpowerman \
	test \
	plmpower

if WITH_HTTPPOWER
SUBDIRS += httppower
endif

# Create run dir for powerman.  Chown will fail if non root - ignore it.
install-data-local:
	-$(top_srcdir)/config/install-sh -m 755 -d \
		$(DESTDIR)$(localstatedir)/run
	-$(top_srcdir)/config/install-sh -o $(RUN_AS_USER) -m 755 -d \
		$(DESTDIR)$(localstatedir)/run/powerman

EXTRA_DIST = DISCLAIMER META 
