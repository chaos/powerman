#!/bin/sh

test_description='Test Powerman list query'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11032

test_expect_success 'create test powerman.conf' '
	cat >powerman.conf <<-EOT
	include "$vpcdev"
	listen "$testaddr"
	device "test0" "vpc" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d >/dev/null
'
test_expect_success 'powerman -l works' '
	$powerman -h $testaddr -l >list.out &&
	echo "t[0-15]" >list.exp &&
	test_cmp list.exp list.out
'
test_expect_success 'powerman -l fails with targets' '
	test_must_fail $powerman -h $testaddr -l t2 2>list.err
'
test_expect_success 'and a useful message was printed on stderr' '
	grep "does not accept targets" list.err
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
