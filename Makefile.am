SUBDIRS = \
	etc \
	man \
	heartbeat \
	src \
	test

# Create run dir for powerman.  Chown will fail if non root - ignore it.
install-data-local:
	-$(top_srcdir)/config/install-sh -m 755 -d \
		$(DESTDIR)$(localstatedir)/run
	-$(top_srcdir)/config/install-sh -o $(RUN_AS_USER) -m 755 -d \
		$(DESTDIR)$(localstatedir)/run/powerman

EXTRA_DIST = \
	DISCLAIMER \
	examples/powerman_el72.spec \
	config/tap-driver.sh \
	scripts/debbuild.sh

export DEB_BUILD_OPTIONS ?= nocheck terse
deb: debian scripts/debbuild.sh
	+@$(top_srcdir)/scripts/debbuild.sh $(abs_top_srcdir)
