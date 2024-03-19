#!/bin/sh

test_description='Cover redfishpower specific configurations'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
redfishdir=$SHARNESS_BUILD_DIRECTORY/src/redfishpower
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices
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
# redfishpower fail hosts coverage
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (failhosts)' '
	cat >powerman_fail_hosts.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-cray-r272z30.dev"
	device "d0" "redfishpower-cray-r272z30" "$redfishdir/redfishpower -h t[0-15] --test-mode --test-fail-power-cmd-hosts=t[8-15] |&"
	node "t[0-15]" "d0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (failhosts)' '
	$powermand -Y -c powerman_fail_hosts.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows t[0-7] off, t[8-15] unknown' '
	$powerman -h $testaddr -q >test_failhosts_query.out &&
	makeoutput "" "t[0-7]" "t[8-15]" >test_failhosts_query.exp &&
	test_cmp test_failhosts_query.exp test_failhosts_query.out
'
test_expect_success 'powerman -1 t[0-15] completes' '
	$powerman -h $testaddr -1 t[0-15] >test_failhosts_on.out &&
	echo Command completed successfully >test_failhosts_on.exp &&
	test_cmp test_failhosts_on.exp test_failhosts_on.out
'
test_expect_success 'powerman -q shows t[0-7] on' '
	$powerman -h $testaddr -q >test_failhosts_query2.out &&
	makeoutput "t[0-7]" "" "t[8-15]" >test_failhosts_query2.exp &&
	test_cmp test_failhosts_query2.exp test_failhosts_query2.out
'
test_expect_success 'stop powerman daemon (failhosts)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower setplugs coverage
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (setplugs)' '
	cat >powerman_setplugs.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setplugs.dev"
	device "d0" "redfishpower-setplugs" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setplugs)' '
	$powermand -Y -c powerman_setplugs.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_setplugs_query.out &&
	makeoutput "" "t[0-15]" "" >test_setplugs_query.exp &&
	test_cmp test_setplugs_query.exp test_setplugs_query.out
'
test_expect_success 'powerman -T -q shows plugs are being used' '
	$powerman -h $testaddr -T -q > test_setplugs_query_T.out &&
	grep "Node15: off" test_setplugs_query_T.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >test_setplugs_on.out &&
	echo Command completed successfully >test_setplugs_on.exp &&
	test_cmp test_setplugs_on.exp test_setplugs_on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_setplugs_query2.out &&
	makeoutput "t[0-15]" "" "" >test_setplugs_query2.exp &&
	test_cmp test_setplugs_query2.exp test_setplugs_query2.out
'
test_expect_success 'stop powerman daemon (setplugs)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower setpath coverage
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (setpath)' '
	cat >powerman_setpath.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setpath.dev"
	device "d0" "redfishpower-setpath" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setpath' '
	$powermand -Y -c powerman_setpath.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_setpath_query.out &&
	makeoutput "" "t[0-15]" "" >test_setpath_query.exp &&
	test_cmp test_setpath_query.exp test_setpath_query.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >test_setpath_on.out &&
	echo Command completed successfully >test_setpath_on.exp &&
	test_cmp test_setpath_on.exp test_setpath_on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_setpath_query2.out &&
	makeoutput "t[0-15]" "" "" >test_setpath_query2.exp &&
	test_cmp test_setpath_query2.exp test_setpath_query2.out
'
test_expect_success 'stop powerman daemon (setpath)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

# Note, verbose output can mess up the device script's interpretation of power status,
# so restart powermand with verbose mode to run telemetry specific tests.
test_expect_success 'create powerman.conf for 16 cray redfish nodes (setpath2)' '
	cat >powerman_setpath2.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setpath.dev"
	device "d0" "redfishpower-setpath" "$redfishdir/redfishpower -h t[0-15] --test-mode -vv |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setpath2)' '
	$powermand -Y -c powerman_setpath2.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
# Note: redfishpower-setpath.dev sets different paths for different
# nodes.  Following tests check that correct path has been used.
test_expect_success 'powerman -T -1 shows correct path being used (t0)' '
	$powerman -h $testaddr -T -1 t0 > test_setpath2_on_T1.out &&
	grep "plugname=Node0" test_setpath2_on_T1.out | grep "path=" | grep Group0
'
test_expect_success 'powerman -T -1 shows correct path being used (t5)' '
	$powerman -h $testaddr -T -1 t5 > test_setpath2_on_T2.out &&
	grep "plugname=Node5" test_setpath2_on_T2.out | grep "path=" | grep Group1
'
test_expect_success 'powerman -T -1 shows correct path being used (t10)' '
	$powerman -h $testaddr -T -1 t10 > test_setpath2_on_T3.out &&
	grep "plugname=Node10" test_setpath2_on_T3.out | grep "path=" | grep Default
'
test_expect_success 'stop powerman daemon (setpath2)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
