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

test_done

# vi: set ft=sh
