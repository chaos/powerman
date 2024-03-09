#!/bin/sh

test_description='Test Powerman diag'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
redfishdir=$SHARNESS_BUILD_DIRECTORY/src/redfishpower
testdevicesdir=$SHARNESS_TEST_SRCDIR/etc

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11035

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

#
# we will use redfishpower for this testing because it
# contains an option for failing specific hosts
#
# note that redfishpower does not natively support power cycle, reset,
# beacon on, or beacon off.  So in the test device files
#
# redfishpower-setresult-all.dev
# redfishpower-setresult-range.dev
# redfishpower-setresult-singlet.dev
#
# power cycle, power reset, beacon on, and beacon off are "fake" implemented
# by just re-using power on or power off.
#

test_expect_success 'create test powerman.conf' '
	cat >powerman_all.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setresult-range.dev"
	device "test0" "redfishpower-setresult" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman_all.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr --diag -q >queryA1.out &&
	makeoutput "" "t[0-15]" "" >queryA1.exp &&
	test_cmp queryA1.exp queryA1.out
'
test_expect_success 'powerman --diag -1 t[0-15] works' '
	$powerman -h $testaddr --diag -1 t[0-15] >onA1.out &&
	echo Command completed successfully >onA1.exp &&
	test_cmp onA1.exp onA1.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr --diag -q >queryA2.out &&
	makeoutput "t[0-15]" "" "" >queryA2.exp &&
	test_cmp queryA2.exp queryA2.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr --diag -0 t[0-15] >offA1.out &&
	echo Command completed successfully >offA1.exp &&
	test_cmp offA1.exp offA1.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr --diag -q >queryA3.out &&
	makeoutput "" "t[0-15]" "" >queryA3.exp &&
	test_cmp queryA3.exp queryA3.out
'
test_expect_success 'powerman --diag -1 t[0-7] works' '
	$powerman -h $testaddr --diag -1 t[0-7] >onA2.out &&
	echo Command completed successfully >onA2.exp &&
	test_cmp onA2.exp onA2.out
'
test_expect_success 'powerman -q shows t[0-7] on' '
	$powerman -h $testaddr --diag -q >queryA4.out &&
	makeoutput "t[0-7]" "t[8-15]" "" >queryA4.exp &&
	test_cmp queryA4.exp queryA4.out
'
test_expect_success 'powerman -0 t[0-7] works' '
	$powerman -h $testaddr --diag -0 t[0-7] >offA2.out &&
	echo Command completed successfully >offA2.exp &&
	test_cmp offA2.exp offA2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr --diag -q >queryA5.out &&
	makeoutput "" "t[0-15]" "" >queryA5.exp &&
	test_cmp queryA5.exp queryA5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# t7,t15 set to bad, so will not work with anything below
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes and bad host' '
	cat >powerman_all_bad_host.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setresult-range.dev"
	device "test0" "redfishpower-setresult" "$redfishdir/redfishpower -h t[0-15] --test-mode --test-fail-power-cmd-hosts=t7,t15 |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman_all_bad_host.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows t[0-6,8-14] off, t[7,15] unknown' '
	$powerman -h $testaddr --diag -q >queryAF1.out &&
	echo t[7,15]: error >queryAF1.exp &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >>queryAF1.exp &&
	test_cmp queryAF1.exp queryAF1.out
'
test_expect_success 'powerman --diag -1 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr --diag -1 t[0-15] >onAF1.out &&
	echo t[7,15]: error >onAF1.exp &&
	echo Command completed with errors >>onAF1.exp &&
	test_cmp onAF1.exp onAF1.out
'
test_expect_success 'powerman -q shows t[0-6,8-14] on, t[7,15] unknown' '
	$powerman -h $testaddr --diag -q >queryAF2.out &&
	echo t[7,15]: error >queryAF2.exp &&
	makeoutput "t[0-6,8-14]" "" "t[7,15]" >>queryAF2.exp &&
	test_cmp queryAF2.exp queryAF2.out
'
test_expect_success 'powerman -0 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr --diag -0 t[0-15] >offAF1.out &&
	echo t[7,15]: error >offAF1.exp &&
	echo Command completed with errors >>offAF1.exp &&
	test_cmp offAF1.exp offAF1.out
'
test_expect_success 'powerman -q shows t[0-6,8-14] off, t[7,15] unknown' '
	$powerman -h $testaddr --diag -q >queryAF3.out &&
	echo t[7,15]: error >queryAF3.exp &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >>queryAF3.exp &&
	test_cmp queryAF3.exp queryAF3.out
'
test_expect_success 'powerman --diag -1 t[0-7] fails' '
	test_must_fail $powerman -h $testaddr --diag -1 t[0-7] >onAF2.out &&
	echo t7: error >onAF2.exp &&
	echo Command completed with errors >>onAF2.exp &&
	test_cmp onAF2.exp onAF2.out
'
test_expect_success 'powerman -q shows t[0-6] on' '
	$powerman -h $testaddr --diag -q >queryAF4.out &&
	echo t[7,15]: error >queryAF4.exp &&
	makeoutput "t[0-6]" "t[8-14]" "t[7,15]" >>queryAF4.exp &&
	test_cmp queryAF4.exp queryAF4.out
'
test_expect_success 'powerman -0 t[0-7] fails' '
	test_must_fail $powerman -h $testaddr --diag -0 t[0-7] >offAF2.out &&
	echo t7: error >offAF2.exp &&
	echo Command completed with errors >>offAF2.exp &&
	test_cmp offAF2.exp offAF2.out
'
test_expect_success 'powerman -q shows t[0-6,8-14] off, t[7,15] unknown' '
	$powerman -h $testaddr --diag -q >queryAF5.out &&
	echo t[7,15]: error >queryAF5.exp &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >>queryAF5.exp &&
	test_cmp queryAF5.exp queryAF5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
