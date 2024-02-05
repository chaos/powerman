#!/bin/sh

test_description='Check FreeIPMI ipmipower device'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
ipmipower=$SHARNESS_BUILD_DIRECTORY/t/simulators/ipmipower
ipmipowerdev=$SHARNESS_TEST_SRCDIR/../etc/devices/ipmipower.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11024


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman.conf one ipmipower device (16 nodes)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$ipmipowerdev"
	device "d0" "ipmipower" "$ipmipower -h t[0-15] |&"
	node "t[0-15]" "d0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-15]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t[0,8] works' '
	$powerman -h $testaddr -1 t[0,8] >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows t[0,8] on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0,8]" "t[1-7,9-15]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t[0,8] works' '
	$powerman -h $testaddr -0 t[0,8] >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "" "t[0-15]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "t[0-15]" "" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr -0 t[0-15] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "" "t[0-15]" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
