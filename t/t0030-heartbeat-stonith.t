#!/bin/sh

test_description='Check heartbeat stonith script for powerman'

. `dirname $0`/sharness.sh

export pm_sim=$SHARNESS_TEST_SRCDIR/scripts/pm-sim.sh
export stonith=$SHARNESS_TEST_SRCDIR/../heartbeat/powerman

echo "Command completed successfully" >success.exp

is_successful() {
	test_cmp success.exp $1
}

# The stonith script is a plugin to heartbeat(8) which enables high
# availability configurations to use powerman to power off members of
# a failover pair.  The plugin is just a shell script that behaves in
# a pre-defined way.  These tests verify that the shell script exhibits
# the expected behaviors and does the right thing when powerman fails.
# It does so by setting $PM_OVERRIDE to point to a script that emulates
# powerman, rather than by setting up a real powerman environment in test.
# The script makes it possible to alter powerman's behavior in ways that
# would be inconvenient in the real thing.

test_expect_success 'reset simulated state' '
	rm -f pm-sim.data
'
test_expect_success 'single plug: stonith on t works' '
	PM_OVERRIDE=$pm_sim PM_VERBOSE=1 \
	    $stonith on t >on.out 2>on.err &&
	is_successful on.out &&
	cat >on_err.exp <<-EOT &&
	powerman-stonith: $pm_sim -1 t, rc=0
	EOT
	test_cmp on_err.exp on.err
'
test_expect_success 'single plug stonith off t works' '
	PM_OVERRIDE=$pm_sim PM_VERBOSE=1 \
	    $stonith off t >off.out 2>off.err &&
	is_successful off.out &&
	cat >off_err.exp <<-EOT &&
	powerman-stonith: $pm_sim -0 t, rc=0
	powerman-stonith: $pm_sim -q t, state=off, rc=0
	EOT
	test_cmp off_err.exp off.err
'
test_expect_success 'reset simulated state' '
	rm -f pm-sim.data
'
test_expect_success 'aliased plug: stonith on t works' '
	PM_OVERRIDE="$pm_sim -A" PM_VERBOSE=1 \
	    $stonith on t >onA.out 2>onA.err &&
	is_successful onA.out &&
	cat >onA_err.exp <<-EOT &&
	powerman-stonith: $pm_sim -A -1 t, rc=0
	EOT
	test_cmp onA_err.exp onA.err
'
test_expect_success 'aliased plug: stonith off t works' '
	PM_OVERRIDE="$pm_sim -A" PM_VERBOSE=1 \
	    $stonith off t >offA.out 2>offA.err &&
	is_successful offA.out &&
	cat >offA_err.exp <<-EOT &&
	powerman-stonith: $pm_sim -A -0 t, rc=0
	powerman-stonith: $pm_sim -A -q t, state=off, rc=0
	EOT
	test_cmp offA_err.exp offA.err
'
test_expect_success 'reset simulated state' '
	rm -f pm-sim.data
'
test_expect_success 'single plug stonith on t works' '
	PM_OVERRIDE=$pm_sim PM_VERBOSE=1 \
	    $stonith on t >on2.out 2>on2.err &&
	is_successful on2.out &&
	cat >on2_err.exp <<-EOT &&
	powerman-stonith: $pm_sim -1 t, rc=0
	EOT
	test_cmp on2_err.exp on2.err
'
test_expect_success 'single plug simulated off failure is detected' '
	(export PM_OVERRIDE="$pm_sim -r -s -t"; export PM_VERBOSE=1; \
	    test_must_fail $stonith off t) >off_fail.out 2>off_fail.err &&
	is_successful off_fail.out &&
	cat >off_fail_err.exp <<-EOT &&
	powerman-stonith: $pm_sim -r -s -t -0 t, rc=0
	powerman-stonith: $pm_sim -r -s -t -q t, state=unknown, rc=1
	EOT
	test_cmp off_fail_err.exp off_fail.err
'
test_expect_success 'reset simulated state' '
	rm -f pm-sim.data
'
test_expect_success 'aliased plug stonith on t works' '
	PM_OVERRIDE="$pm_sim -A" PM_VERBOSE=1 \
	    $stonith on t >onA2.out 2>onA2.err &&
	is_successful onA2.out &&
	cat >onA2_err.exp <<-EOT &&
	powerman-stonith: $pm_sim -A -1 t, rc=0
	EOT
	test_cmp onA2_err.exp onA2.err
'
test_expect_success 'aliased plug simulated off failure is detected' '
	(export PM_OVERRIDE="$pm_sim -A -r -s -t"; export PM_VERBOSE=1; \
	    test_must_fail $stonith off t) >offA_fail.out 2>offA_fail.err &&
	is_successful offA_fail.out &&
	cat >offA_fail_err.exp <<-EOT &&
	powerman-stonith: $pm_sim -A -r -s -t -0 t, rc=0
	powerman-stonith: $pm_sim -A -r -s -t -q t, state=unknown, rc=1
	EOT
	test_cmp offA_fail_err.exp offA_fail.err
'

test_done

# vi: set ft=sh
