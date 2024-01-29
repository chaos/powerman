#!/bin/sh

test_description='Check Insteon PLM scripts with plmpower'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
plmpower=$SHARNESS_BUILD_DIRECTORY/src/plmpower/plmpower
plmdev=$SHARNESS_TEST_SRCDIR/../etc/devices/plmpower.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11019


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

# Reminder: X10 devices have no ability to report power status so
# their state is always "unknown".
test_expect_success 'create powerman.conf with plmpower (t0 Insteon, t1 X10)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$plmdev"
	device "d0" "plmpower" "$plmpower -T |&"
	node "t0"  "d0" "aa.bb.cc"
	node "t1"  "d0" "G1"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows t0 off, t1 unknown' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t0" "t1" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t0 works' '
	$powerman -h $testaddr -1 t0 >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows t0 on, t1 unknown' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t0" "" "t1" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -c t0 works' '
	$powerman -h $testaddr -c t0 >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows t0 on, t1 unknown' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t0" "" "t1" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t0 works' '
	$powerman -h $testaddr -0 t0 >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows t0 off, t1 unknown off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t0" "t1" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -1 t[0-1] works' '
	$powerman -h $testaddr -1 t[0-1] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows t0 on, t1 uknonwn' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t0" "" "t1" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -c t[0-1] works' '
	$powerman -h $testaddr -c t[0-1] >cycle2.out &&
	echo Command completed successfully >cycle2.exp &&
	test_cmp cycle2.exp cycle2.out
'
test_expect_success 'powerman -q shows t0 on, t1 unknown' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t0" "" "t1" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -0 t[0-1] works' '
	$powerman -h $testaddr -0 t[0-1] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows t0 off, t1 unknown' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "" "t0" "t1" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
