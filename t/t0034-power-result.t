#!/bin/sh

test_description='Test Powerman power result'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
redfishdir=$SHARNESS_BUILD_DIRECTORY/src/redfishpower
testdevicesdir=$SHARNESS_TEST_SRCDIR/etc

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11034

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

#
# "all" scripts
#
# on_all, off_all, cycle_all, reset_all
#

test_expect_success 'create test powerman.conf (setresult all scripts)' '
	cat >powerman_all.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setresult-all.dev"
	device "test0" "redfishpower-setresult" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setresult all scripts)' '
	$powermand -c powerman_all.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryA1.out &&
	makeoutput "" "t[0-15]" "" >queryA1.exp &&
	test_cmp queryA1.exp queryA1.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >onA1.out &&
	echo Command completed successfully >onA1.exp &&
	test_cmp onA1.exp onA1.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >queryA2.out &&
	makeoutput "t[0-15]" "" "" >queryA2.exp &&
	test_cmp queryA2.exp queryA2.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr -0 t[0-15] >offA1.out &&
	echo Command completed successfully >offA1.exp &&
	test_cmp offA1.exp offA1.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryA3.out &&
	makeoutput "" "t[0-15]" "" >queryA3.exp &&
	test_cmp queryA3.exp queryA3.out
'
test_expect_success 'powerman -c t[0-15] works' '
	$powerman -h $testaddr -c t[0-15] >cycleA1.out &&
	echo Command completed successfully >cycleA1.exp &&
	test_cmp cycleA1.exp cycleA1.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >queryA4.out &&
	makeoutput "t[0-15]" "" "" >queryA4.exp &&
	test_cmp queryA4.exp queryA4.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr -0 t[0-15] >offA2.out &&
	echo Command completed successfully >offA2.exp &&
	test_cmp offA2.exp offA2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryA5.out &&
	makeoutput "" "t[0-15]" "" >queryA5.exp &&
	test_cmp queryA5.exp queryA5.out
'
test_expect_success 'powerman -r t[0-15] works' '
	$powerman -h $testaddr -r t[0-15] >resetA1.out &&
	echo Command completed successfully >resetA1.exp &&
	test_cmp resetA1.exp resetA1.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >queryA6.out &&
	makeoutput "t[0-15]" "" "" >queryA6.exp &&
	test_cmp queryA6.exp queryA6.out
'
test_expect_success 'stop powerman daemon (setresult all scripts)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# "all" scripts
#
# on_all, off_all, cycle_all, reset_all
#
# t15 set to bad, so will not work with anything below
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes and bad host (setresult all scripts bad host)' '
	cat >powerman_all_bad_host.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setresult-all.dev"
	device "test0" "redfishpower-setresult" "$redfishdir/redfishpower -h t[0-15] --test-mode --test-fail-power-cmd-hosts=t15 |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setresult all scripts bad host)' '
	$powermand -Y -c powerman_all_bad_host.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >queryAF1.out &&
	makeoutput "" "t[0-14]" "t15" >queryAF1.exp &&
	test_cmp queryAF1.exp queryAF1.out
'
test_expect_success 'powerman -1 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -1 t[0-15] >onAF1.out &&
	echo Command completed with errors >onAF1.exp &&
	test_cmp onAF1.exp onAF1.out
'
test_expect_success 'powerman -q shows t[0-14] on, t15 unknown' '
	$powerman -h $testaddr -q >queryAF2.out &&
	makeoutput "t[0-14]" "" "t15" >queryAF2.exp &&
	test_cmp queryAF2.exp queryAF2.out
'
test_expect_success 'powerman -0 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[0-15] >offAF1.out &&
	echo Command completed with errors >offAF1.exp &&
	test_cmp offAF1.exp offAF1.out
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >queryAF3.out &&
	makeoutput "" "t[0-14]" "t15" >queryAF3.exp &&
	test_cmp queryAF3.exp queryAF3.out
'
test_expect_success 'powerman -c t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -c t[0-15] >cycleAF1.out &&
	echo Command completed with errors >cycleAF1.exp &&
	test_cmp cycleAF1.exp cycleAF1.out
'
test_expect_success 'powerman -q shows t[0-14] on, t15 unknown' '
	$powerman -h $testaddr -q >queryAF4.out &&
	makeoutput "t[0-14]" "" "t15" >queryAF4.exp &&
	test_cmp queryAF4.exp queryAF4.out
'
test_expect_success 'powerman -0 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[0-15] >offAF2.out &&
	echo Command completed with errors >offAF2.exp &&
	test_cmp offAF2.exp offAF2.out
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >queryAF5.out &&
	makeoutput "" "t[0-14]" "t15" >queryAF5.exp &&
	test_cmp queryAF5.exp queryAF5.out
'
test_expect_success 'powerman -r t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -r t[0-15] >resetAF1.out &&
	echo Command completed with errors >resetAF1.exp &&
	test_cmp resetAF1.exp resetAF1.out
'
test_expect_success 'powerman -q shows t[0-14] on, t15 unknown' '
	$powerman -h $testaddr -q >queryAF6.out &&
	makeoutput "t[0-14]" "" "t15" >queryAF6.exp &&
	test_cmp queryAF6.exp queryAF6.out
'
test_expect_success 'stop powerman daemon (setresult all scripts bad host)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# "range" scripts
#
# on_range, off_range, cycle_range, reset_range, beacon_on_range, beacon_off_range
#

test_expect_success 'create test powerman.conf (setresult range scripts)' '
	cat >powerman_range.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setresult-range.dev"
	device "test0" "redfishpower-setresult" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setresult range scripts)' '
	$powermand -c powerman_range.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryR1.out &&
	makeoutput "" "t[0-15]" "" >queryR1.exp &&
	test_cmp queryR1.exp queryR1.out
'
test_expect_success 'powerman -1 t[8-15] works' '
	$powerman -h $testaddr -1 t[8-15] >onR1.out &&
	echo Command completed successfully >onR1.exp &&
	test_cmp onR1.exp onR1.out
'
test_expect_success 'powerman -q shows t[8-15] on' '
	$powerman -h $testaddr -q >queryR2.out &&
	makeoutput "t[8-15]" "t[0-7]" "" >queryR2.exp &&
	test_cmp queryR2.exp queryR2.out
'
test_expect_success 'powerman -0 t[8-15] works' '
	$powerman -h $testaddr -0 t[8-15] >offR1.out &&
	echo Command completed successfully >offR1.exp &&
	test_cmp offR1.exp offR1.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryR3.out &&
	makeoutput "" "t[0-15]" "" >queryR3.exp &&
	test_cmp queryR3.exp queryR3.out
'
test_expect_success 'powerman -c t[8-15] works' '
	$powerman -h $testaddr -c t[8-15] >cycleR1.out &&
	echo Command completed successfully >cycleR1.exp &&
	test_cmp cycleR1.exp cycleR1.out
'
test_expect_success 'powerman -q shows t[8-15] on' '
	$powerman -h $testaddr -q >queryR4.out &&
	makeoutput "t[8-15]" "t[0-7]" "" >queryR4.exp &&
	test_cmp queryR4.exp queryR4.out
'
test_expect_success 'powerman -0 t[8-15] works' '
	$powerman -h $testaddr -0 t[8-15] >offR2.out &&
	echo Command completed successfully >offR2.exp &&
	test_cmp offR2.exp offR2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryR5.out &&
	makeoutput "" "t[0-15]" "" >queryR5.exp &&
	test_cmp queryR5.exp queryR5.out
'
test_expect_success 'powerman -r t[8-15] works' '
	$powerman -h $testaddr -r t[8-15] >resetR1.out &&
	echo Command completed successfully >resetR1.exp &&
	test_cmp resetR1.exp resetR1.out
'
test_expect_success 'powerman -q shows t[8-15] on' '
	$powerman -h $testaddr -q >queryR6.out &&
	makeoutput "t[8-15]" "t[0-7]" "" >queryR6.exp &&
	test_cmp queryR6.exp queryR6.out
'
test_expect_success 'powerman -f t[8-15] works' '
	$powerman -h $testaddr -f t[8-15] >flashR1.out &&
	echo Command completed successfully >flashR1.exp &&
	test_cmp flashR1.exp flashR1.out
'
test_expect_success 'powerman -u t[8-15] works' '
	$powerman -h $testaddr -u t[8-15] >unflashR1.out &&
	echo Command completed successfully >unflashR1.exp &&
	test_cmp unflashR1.exp unflashR1.out
'
test_expect_success 'stop powerman daemon (setresult range scripts)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# "range" scripts
#
# on_range, off_range, cycle_range, reset_range, beacon_on_range, beacon_off_range
#
# t15 set to bad, so will not work with anything below
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes and bad host (setresult range scripts bad host)' '
	cat >powerman_range_bad_host.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setresult-range.dev"
	device "test0" "redfishpower-setresult" "$redfishdir/redfishpower -h t[0-15] --test-mode --test-fail-power-cmd-hosts=t15 |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setresult range scripts bad host)' '
	$powermand -Y -c powerman_range_bad_host.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >queryRF1.out &&
	makeoutput "" "t[0-14]" "t15" >queryRF1.exp &&
	test_cmp queryRF1.exp queryRF1.out
'
test_expect_success 'powerman -1 t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -1 t[8-15] >onRF1.out &&
	echo Command completed with errors >onRF1.exp &&
	test_cmp onRF1.exp onRF1.out
'
test_expect_success 'powerman -q shows t[8-14] on, t15 unknown' '
	$powerman -h $testaddr -q >queryRF2.out &&
	makeoutput "t[8-14]" "t[0-7]" "t15" >queryRF2.exp &&
	test_cmp queryRF2.exp queryRF2.out
'
test_expect_success 'powerman -0 t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[8-15] >offRF1.out &&
	echo Command completed with errors >offRF1.exp &&
	test_cmp offRF1.exp offRF1.out
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >queryRF3.out &&
	makeoutput "" "t[0-14]" "t15" >queryRF3.exp &&
	test_cmp queryRF3.exp queryRF3.out
'
test_expect_success 'powerman -c t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -c t[8-15] >cycleRF1.out &&
	echo Command completed with errors >cycleRF1.exp &&
	test_cmp cycleRF1.exp cycleRF1.out
'
test_expect_success 'powerman -q shows t[8-14] on, t15 unknown' '
	$powerman -h $testaddr -q >queryRF4.out &&
	makeoutput "t[8-14]" "t[0-7]" "t15" >queryRF4.exp &&
	test_cmp queryRF4.exp queryRF4.out
'
test_expect_success 'powerman -0 t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[8-15] >offRF2.out &&
	echo Command completed with errors >offRF2.exp &&
	test_cmp offRF2.exp offRF2.out
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >queryRF5.out &&
	makeoutput "" "t[0-14]" "t15" >queryRF5.exp &&
	test_cmp queryRF5.exp queryRF5.out
'
test_expect_success 'powerman -r t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -r t[8-15] >resetRF1.out &&
	echo Command completed with errors >resetRF1.exp &&
	test_cmp resetRF1.exp resetRF1.out
'
test_expect_success 'powerman -q shows t[8-14] on, t15 unknown' '
	$powerman -h $testaddr -q >queryRF6.out &&
	makeoutput "t[8-14]" "t[0-7]" "t15" >queryRF6.exp &&
	test_cmp queryRF6.exp queryRF6.out
'
test_expect_success 'powerman -f t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -f t[8-15] >flashRF1.out &&
	echo Command completed with errors >flashRF1.exp &&
	test_cmp flashRF1.exp flashRF1.out
'
test_expect_success 'powerman -u t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -u t[8-15] >unflashRF1.out &&
	echo Command completed with errors >unflashRF1.exp &&
	test_cmp unflashRF1.exp unflashRF1.out
'
test_expect_success 'stop powerman daemon (setresult range scripts bad host)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# "singlet" scripts
#
# on, off, cycle, reset, beacon_on, beacon_off
#

test_expect_success 'create test powerman.conf (setresult singlet scripts)' '
	cat >powerman_singlet.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setresult-singlet.dev"
	device "test0" "redfishpower-setresult" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setresult singlet scripts)' '
	$powermand -c powerman_singlet.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryS1.out &&
	makeoutput "" "t[0-15]" "" >queryS1.exp &&
	test_cmp queryS1.exp queryS1.out
'
test_expect_success 'powerman -1 t[8,15] works' '
	$powerman -h $testaddr -1 t[8,15] >onS1.out &&
	echo Command completed successfully >onS1.exp &&
	test_cmp onS1.exp onS1.out
'
test_expect_success 'powerman -q shows t[8,15] on' '
	$powerman -h $testaddr -q >queryS2.out &&
	makeoutput "t[8,15]" "t[0-7,9-14]" "" >queryS2.exp &&
	test_cmp queryS2.exp queryS2.out
'
test_expect_success 'powerman -0 t[8,15] works' '
	$powerman -h $testaddr -0 t[8,15] >offS1.out &&
	echo Command completed successfully >offS1.exp &&
	test_cmp offS1.exp offS1.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryS3.out &&
	makeoutput "" "t[0-15]" "" >queryS3.exp &&
	test_cmp queryS3.exp queryS3.out
'
test_expect_success 'powerman -c t[8,15] works' '
	$powerman -h $testaddr -c t[8,15] >cycleS1.out &&
	echo Command completed successfully >cycleS1.exp &&
	test_cmp cycleS1.exp cycleS1.out
'
test_expect_success 'powerman -q shows t[8,15] on' '
	$powerman -h $testaddr -q >queryS4.out &&
	makeoutput "t[8,15]" "t[0-7,9-14]" "" >queryS4.exp &&
	test_cmp queryS4.exp queryS4.out
'
test_expect_success 'powerman -0 t[8,15] works' '
	$powerman -h $testaddr -0 t[8,15] >offS2.out &&
	echo Command completed successfully >offS2.exp &&
	test_cmp offS2.exp offS2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryS5.out &&
	makeoutput "" "t[0-15]" "" >queryS5.exp &&
	test_cmp queryS5.exp queryS5.out
'
test_expect_success 'powerman -r t[8,15] works' '
	$powerman -h $testaddr -r t[8,15] >resetS1.out &&
	echo Command completed successfully >resetS1.exp &&
	test_cmp resetS1.exp resetS1.out
'
test_expect_success 'powerman -q shows t[8,15] on' '
	$powerman -h $testaddr -q >queryS6.out &&
	makeoutput "t[8,15]" "t[0-7,9-14]" "" >queryS6.exp &&
	test_cmp queryS6.exp queryS6.out
'
test_expect_success 'powerman -f t[8,15] works' '
	$powerman -h $testaddr -f t[8,15] >flashS1.out &&
	echo Command completed successfully >flashS1.exp &&
	test_cmp flashS1.exp flashS1.out
'
test_expect_success 'powerman -u t[8,15] works' '
	$powerman -h $testaddr -u t[8,15] >unflashS1.out &&
	echo Command completed successfully >unflashS1.exp &&
	test_cmp unflashS1.exp unflashS1.out
'
test_expect_success 'stop powerman daemon (setresult singlet scripts)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# "singlet" scripts
#
# on, off, cycle, reset, beacon_on, beacon_off
#
# t15 set to bad, so will not work with anything below
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes and bad host (setresult singlet scripts bad host)' '
	cat >powerman_singlet_bad_host.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setresult-singlet.dev"
	device "test0" "redfishpower-setresult" "$redfishdir/redfishpower -h t[0-15] --test-mode --test-fail-power-cmd-hosts=t15 |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setresult singlet scripts bad host)' '
	$powermand -Y -c powerman_singlet_bad_host.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >querySF1.out &&
	makeoutput "" "t[0-14]" "t15" >querySF1.exp &&
	test_cmp querySF1.exp querySF1.out
'
test_expect_success 'powerman -1 t[8,15] fails' '
	test_must_fail $powerman -h $testaddr -1 t[8,15] >onSF1.out &&
	echo Command completed with errors >onSF1.exp &&
	test_cmp onSF1.exp onSF1.out
'
test_expect_success 'powerman -q shows t8 on, t15 unknown' '
	$powerman -h $testaddr -q >querySF2.out &&
	makeoutput "t8" "t[0-7,9-14]" "t15" >querySF2.exp &&
	test_cmp querySF2.exp querySF2.out
'
test_expect_success 'powerman -0 t[8,15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[8,15] >offSF1.out &&
	echo Command completed with errors >offSF1.exp &&
	test_cmp offSF1.exp offSF1.out
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >querySF3.out &&
	makeoutput "" "t[0-14]" "t15" >querySF3.exp &&
	test_cmp querySF3.exp querySF3.out
'
test_expect_success 'powerman -c t[8,15] fails' '
	test_must_fail $powerman -h $testaddr -c t[8,15] >cycleSF1.out &&
	echo Command completed with errors >cycleSF1.exp &&
	test_cmp cycleSF1.exp cycleSF1.out
'
test_expect_success 'powerman -q shows t[8-14] on, t15 unknown' '
	$powerman -h $testaddr -q >querySF4.out &&
	makeoutput "t8" "t[0-7,9-14]" "t15" >querySF4.exp &&
	test_cmp querySF4.exp querySF4.out
'
test_expect_success 'powerman -0 t[8,15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[8,15] >offSF2.out &&
	echo Command completed with errors >offSF2.exp &&
	test_cmp offSF2.exp offSF2.out
'
test_expect_success 'powerman -q shows t[0-14] off, t15 unknown' '
	$powerman -h $testaddr -q >querySF5.out &&
	makeoutput "" "t[0-14]" "t15" >querySF5.exp &&
	test_cmp querySF5.exp querySF5.out
'
test_expect_success 'powerman -r t[8,15] fails' '
	test_must_fail $powerman -h $testaddr -r t[8,15] >resetSF1.out &&
	echo Command completed with errors >resetSF1.exp &&
	test_cmp resetSF1.exp resetSF1.out
'
test_expect_success 'powerman -q shows t[8-14] on, t15 unknown' '
	$powerman -h $testaddr -q >querySF6.out &&
	makeoutput "t8" "t[0-7,9-14]" "t15" >querySF6.exp &&
	test_cmp querySF6.exp querySF6.out
'
test_expect_success 'powerman -f t[8,15] fails' '
	test_must_fail $powerman -h $testaddr -f t[8,15] >flashSF1.out &&
	echo Command completed with errors >flashSF1.exp &&
	test_cmp flashSF1.exp flashSF1.out
'
test_expect_success 'powerman -u t[8,15] fails' '
	test_must_fail $powerman -h $testaddr -u t[8,15] >unflashSF1.out &&
	echo Command completed with errors >unflashSF1.exp &&
	test_cmp unflashSF1.exp unflashSF1.out
'
test_expect_success 'stop powerman daemon (setresult singlet scripts bad host)' '
	kill -15 $(cat powermand.pid) &&
	wait
'



test_done

# vi: set ft=sh
