if HAVE_SYSTEMD
systemdsystemunit_SCRIPTS = powerman.service
systemdtmpdir_ddir = ${prefix}/lib/tmpfiles.d
systemdtmpdir_d_SCRIPTS = tmpfiles.d/powerman.conf
else
initconfdir = $(sysconfdir)/init.d
initconf_SCRIPTS = powerman
endif

EXTRA_DIST = powerman.service powerman tmpfiles.d/powerman.conf
