#!/bin/sh

test_description='Check redfish-supermicro device which uses httppower'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
simdir=$SHARNESS_BUILD_DIRECTORY/t/simulators
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11027


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman.conf for 5 redfish-supermicro nodes' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfish-supermicro.dev"
	device "d0" "redfish-supermicro" "$simdir/redfish-httppower |&"
	device "d1" "redfish-supermicro" "$simdir/redfish-httppower |&"
	device "d2" "redfish-supermicro" "$simdir/redfish-httppower |&"
	device "d3" "redfish-supermicro" "$simdir/redfish-httppower |&"
	device "d4" "redfish-supermicro" "$simdir/redfish-httppower |&"
	node "t0" "d0" "t0"
	node "t1" "d1" "t1"
	node "t2" "d2" "t2"
	node "t3" "d3" "t3"
	node "t4" "d4" "t4"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-4]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t[0-4] works' '
	$powerman -h $testaddr -1 t[0-4] >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0-4]" "" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t[0-4] works' '
	$powerman -h $testaddr -0 t[0-4] >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "" "t[0-4]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -c t[0-4] works' '
	$powerman -h $testaddr -c t[0-4] >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "t[0-4]" "" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
