if HAVE_SYSTEMD
systemdsystemunit_SCRIPTS = powerman.service
EXTRA_DIST = $(systemdsystemunit_SCRIPTS)
else
initconfdir = $(sysconfdir)/init.d
initconf_SCRIPTS = powerman
EXTRA_DIST = $(initconf_SCRIPTS)
endif

