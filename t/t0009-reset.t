#!/bin/sh

test_description='Test Powerman reset control'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11009

test_expect_success 'create test powerman.conf' '
	cat >powerman.conf <<-EOT
	include "$vpcdev"
	listen "$testaddr"
	device "test0" "vpc" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -r t1 works' '
	$powerman -h $testaddr -r t1 >reset1.out &&
	echo "Command completed successfully" >reset1.exp &&
	test_cmp reset1.exp reset1.out
'
test_expect_success 'powerman -r t[3-5] works' '
	$powerman -h $testaddr -r t[3-5] >reset2.out &&
	echo "Command completed successfully" >reset2.exp &&
	test_cmp reset2.exp reset2.out
'
test_expect_success 'powerman -r with no targets fails with useful error' '
        test_must_fail $powerman -h $testaddr -r 2>notargets.err &&
        grep "Command requires targets" notargets.err
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
