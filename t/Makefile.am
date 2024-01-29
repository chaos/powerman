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
	t0013-baytech-rpc3-nc.t

# make check runs these TAP tests directly (both scripts and programs)
TESTS = \
	$(TESTSCRIPTS)

dist_check_SCRIPTS = \
	$(TESTSCRIPTS)

check-prep:
	$(MAKE)

EXTRA_DIST= \
	aggregate-results.sh \
	sharness.sh
