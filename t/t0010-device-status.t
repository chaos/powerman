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
	device "test1" "vpc" "$vpcd |&"
	node "t[0-3]" "test0"
	node "t[4-7]" "test1"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -d works' '
	$powerman -h $testaddr -d >device.out &&
	cat >device.exp <<-EOT &&
	test0: state=connected reconnects=000 actions=002 type=vpc hosts=t[0-3]
	test1: state=connected reconnects=000 actions=002 type=vpc hosts=t[4-7]
	EOT
	test_cmp device.exp device.out
'
test_expect_success 'run powerman -q t1' '
	$powerman -h $testaddr -q t1 >/dev/null
'
test_expect_success 'powerman -d shows additional action' '
	$powerman -h $testaddr -d >device2.out &&
	cat >device2.exp <<-EOT &&
	test0: state=connected reconnects=000 actions=003 type=vpc hosts=t[0-3]
	test1: state=connected reconnects=000 actions=002 type=vpc hosts=t[4-7]
	EOT
	test_cmp device2.exp device2.out
'
test_expect_success 'running powerman -d t1 returns stats for test0' '
	$powerman -h $testaddr -d t1 >device3.out &&
	cat >device3.exp <<-EOT &&
	test0: state=connected reconnects=000 actions=003 type=vpc hosts=t[0-3]
	EOT
	test_cmp device3.exp device3.out
'
test_expect_success 'running powerman -d t[1,5] returns stats for both' '
	$powerman -h $testaddr -d t[1,5] >device4.out &&
	cat >device4.exp <<-EOT &&
	test0: state=connected reconnects=000 actions=003 type=vpc hosts=t[0-3]
	test1: state=connected reconnects=000 actions=002 type=vpc hosts=t[4-7]
	EOT
	test_cmp device4.exp device4.out
'
# FWIW the device query computes the intersection between plugs and devices
# and the empty set is treated as a valid result, so this is not an error.
# That seems a bit wrong but since this is mainly a test option, maybe not
# worth changing.
test_expect_success 'running powerman -d xyz returns nothing' '
	$powerman -h $testaddr -d xyz >device5.out &&
	test_must_fail test -s device5.out
'

# Should this be somewhere else?
test_expect_success 'powerman -l lists targets' '
	$powerman -h $testaddr -l >list.out &&
	echo "t[0-7]" >list.exp &&
	test_cmp list.exp list.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
