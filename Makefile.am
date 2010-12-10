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
if WITH_SNMPPOWER
SUBDIRS += snmppower
endif
if WITH_H8POWER
SUBDIRS += h8power
endif

# Create run dir for powerman.  Chown will fail if non root - ignore it.
install-data-local:
	-$(top_srcdir)/config/install-sh -m 755 -d \
		$(DESTDIR)$(localstatedir)/run
	-$(top_srcdir)/config/install-sh -o $(RUN_AS_USER) -m 755 -d \
		$(DESTDIR)$(localstatedir)/run/powerman

rpmdirs:
	mkdir -p $(top_builddir)/rpm/SPECS
	mkdir -p $(top_builddir)/rpm/SOURCES
	mkdir -p $(top_builddir)/rpm/BUILD
	mkdir -p $(top_builddir)/rpm/RPMS
	mkdir -p $(top_builddir)/rpm/SRPMS

rpm: dist rpmdirs
	rpmbuild -ta --define "_topdir $(realpath $(top_builddir)/rpm)" $(DIST_ARCHIVES)


EXTRA_DIST = DISCLAIMER META powerman.spec
