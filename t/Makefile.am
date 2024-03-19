AM_TESTS_ENVIRONMENT =

TEST_EXTENSIONS = .t
T_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/config/tap-driver.sh

clean-local:
	rm -fr trash-directory.* test-results .prove *.output

# This list is included in both TESTS and dist_check_SCRIPTS.
TESTSCRIPTS = \
	t0000-sharness.t \
	t0001-powerman-client.t \
	t0002-pluglist.t \
	t0003-api-client.t \
	t0004-status-query.t \
	t0005-telemetry.t \
	t0006-on-off-cycle.t \
	t0007-temperature.t \
	t0008-beacon.t \
	t0009-reset.t \
	t0010-device-status.t \
	t0011-device-timeout.t \
	t0012-device-delay.t \
	t0013-baytech-rpc3-nc.t \
	t0014-baytech-rpc28-nc.t \
	t0015-baytech-rpc3.t \
	t0016-icebox-v3.t \
	t0017-icebox-v2.t \
	t0018-cyclades-pm.t \
	t0019-insteon-plm.t \
	t0020-digital-loggers-lpc.t \
	t0021-remote-powerman.t \
	t0023-sun-microsystems-ilom.t \
	t0024-freeipmi.t \
	t0025-openbmc.t \
	t0026-llnl-sierra-cluster.t \
	t0027-redfish-httppower.t \
	t0028-issues.t \
	t0029-redfish.t \
	t0030-heartbeat-stonith.t \
	t0031-llnl-mcr-cluster.t \
	t0032-list.t \
	t0033-valgrind.t \
	t0034-redfishpower.t

# make check runs these TAP tests directly (both scripts and programs)
TESTS = \
	$(TESTSCRIPTS)

dist_check_SCRIPTS = \
	$(TESTSCRIPTS) \
	scripts/pm-sim.sh

check-prep:
	$(MAKE)

EXTRA_DIST= \
	aggregate-results.sh \
	sharness.sh \
	etc/sierra_plugs.conf \
	etc/mcr_plugs.conf \
	etc/vpc.dev


AM_CFLAGS = @WARNING_CFLAGS@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/libcommon \
	-I$(top_srcdir)/src/liblsd \
	-I$(top_srcdir)/src/powerman

common_ldadd = \
	$(top_builddir)/src/liblsd/liblsd.la \
	$(top_builddir)/src/libcommon/libcommon.la

check_PROGRAMS = \
	simulators/vpcd \
	simulators/baytech \
	simulators/icebox \
	simulators/dli \
	simulators/cyclades \
	simulators/ipmipower \
	simulators/ilom \
	simulators/lom \
	simulators/swpdu \
	simulators/openbmc-httppower \
	simulators/redfish-httppower


simulators_vpcd_SOURCES = simulators/vpcd.c
simulators_vpcd_LDADD = $(common_ldadd)

simulators_baytech_SOURCES = simulators/baytech.c
simulators_baytech_LDADD = $(common_ldadd)

simulators_icebox_SOURCES = simulators/icebox.c
simulators_icebox_LDADD = $(common_ldadd)

simulators_swpdu_SOURCES = simulators/swpdu.c
simulators_swpdu_LDADD = $(common_ldadd)

simulators_dli_SOURCES = simulators/dli.c
simulators_dli_LDADD = $(common_ldadd)

simulators_ilom_SOURCES = simulators/ilom.c
simulators_ilom_LDADD = $(common_ldadd)

simulators_lom_SOURCES = simulators/lom.c
simulators_lom_LDADD = $(common_ldadd)

simulators_cyclades_SOURCES = simulators/cyclades.c
simulators_cyclades_LDADD = $(common_ldadd)

simulators_ipmipower_SOURCES = simulators/ipmipower.c
simulators_ipmipower_LDADD = $(common_ldadd)

simulators_openbmc_httppower_SOURCES = simulators/openbmc-httppower.c
simulators_openbmc_httppower_LDADD = $(common_ldadd)

simulators_redfish_httppower_SOURCES = simulators/redfish-httppower.c
simulators_redfish_httppower_LDADD = $(common_ldadd)
