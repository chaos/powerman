#!/bin/sh

test_description='Check bashfun toy/demo device'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
bash=/bin/sh # (it doesn't require actual bash)
bashfundev=$SHARNESS_TEST_SRCDIR/../etc/devices/bashfun.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11022


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman.conf (1 plug)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$bashfundev"
	device "d0" "bashfun" "$bash |&"
	node "t1" "d0" "1"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows t1 off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t1" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t1 works' '
	$powerman -h $testaddr -1 t1 >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows t1 on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t1" "" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t1 works' '
	$powerman -h $testaddr -0 t1 >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows t1 off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t1" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
