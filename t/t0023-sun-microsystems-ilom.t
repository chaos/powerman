#!/bin/sh

test_description='Check Sun Microsystems ilom and lom devices'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
ilom=$SHARNESS_BUILD_DIRECTORY/t/simulators/ilom
lom=$SHARNESS_BUILD_DIRECTORY/t/simulators/lom
ilomdev=$SHARNESS_TEST_SRCDIR/../etc/devices/ilom.dev
lomdev=$SHARNESS_TEST_SRCDIR/../etc/devices/lom.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11023


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman.conf for lom + ilom devices (6 plugs)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$ilomdev"
	include "$lomdev"
	device "d0" "ilom" "$ilom -p ssh |&"
	device "d1" "ilom" "$ilom -p serial |&"
	device "d2" "ilom" "$ilom -p serial_loggedin |&"
	device "d3" "lom" "$lom -p ssh |&"
	device "d4" "lom" "$lom -p serial |&"
	device "d5" "lom" "$lom -p serial_loggedin |&"
	node "t0" "d0"
	node "t1" "d1"
	node "t2" "d2"
	node "t3" "d3"
	node "t4" "d4"
	node "t5" "d5"
	alias "iloms" "t[0-2]"
	alias "loms" "t[3-5]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-5]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t[0-5] works' '
	$powerman -h $testaddr -1 t[0-5] >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0-5]" "" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -c t[0-5] works' '
	$powerman -h $testaddr -c t[0-5] >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0-5]" "" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t[0-5] works' '
	$powerman -h $testaddr -0 t[0-5] >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-5]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -r iloms works' '
	$powerman -h $testaddr -r iloms >reset.out &&
	echo Command completed successfully >reset.exp &&
	test_cmp reset.exp reset.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "" "t[0-5]" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -r loms fails' '
	test_must_fail $powerman -h $testaddr -r loms
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
