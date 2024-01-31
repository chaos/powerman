#!/bin/sh

test_description='Check Digital Loggers Inc LPC device'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
dli=$SHARNESS_BUILD_DIRECTORY/t/simulators/dli
dlidev=$SHARNESS_TEST_SRCDIR/../etc/devices/dli.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11020


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman.conf for DLI LPC device (8 plugs)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$dlidev"
	device "d0" "dli" "$dli |&"
	node "t[0-7]" "d0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-7]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t0 works' '
	$powerman -h $testaddr -1 t0 >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t0" "t[1-7]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -c t0 works' '
	$powerman -h $testaddr -c t0 >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t0" "t[1-7]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t0 works' '
	$powerman -h $testaddr -0 t0 >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-7]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -1 t[0-7] works' '
	$powerman -h $testaddr -1 t[0-7] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-7]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -c t[0-7] works' '
	$powerman -h $testaddr -c t[0-7] >cycle2.out &&
	echo Command completed successfully >cycle2.exp &&
	test_cmp cycle2.exp cycle2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-7]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -0 t[0-7] works' '
	$powerman -h $testaddr -0 t[0-7] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "" "t[0-7]" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
