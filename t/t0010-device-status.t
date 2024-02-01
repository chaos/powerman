#!/bin/sh

test_description='Test Device status command'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11010

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
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -d works' '
	$powerman -h $testaddr -d >device.out &&
	echo "test0: state=connected reconnects=000 actions=002 type=vpc hosts=t[0-15]" >device.exp &&
	test_cmp device.exp device.out
'
test_expect_success 'run powerman -Q t1' '
	$powerman -h $testaddr -Q t1 >/dev/null
'
test_expect_success 'powerman -d shows additional action' '
	$powerman -h $testaddr -d >device2.out &&
	echo "test0: state=connected reconnects=000 actions=003 type=vpc hosts=t[0-15]" >device2.exp &&
	test_cmp device2.exp device2.out
'
test_expect_success 'running powerman -D t1 returns the same thing' '
	$powerman -h $testaddr -D t1 >device3.out &&
	test_cmp device2.exp device3.out
'
# Should this be somewhere else?
test_expect_success 'powerman -l lists targets' '
	$powerman -h $testaddr -l >list.out &&
	echo "t[0-15]" >list.exp &&
	test_cmp list.exp list.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
