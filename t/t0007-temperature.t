#!/bin/sh

test_description='Test Powerman temperature query'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11007

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
test_expect_success 'powerman -t works' '
	$powerman -h $testaddr -t >query_all.out &&
	cat >query_all.exp <<-EOT &&
	t0: 83
	t1: 84
	t2: 85
	t3: 86
	t4: 87
	t5: 88
	t6: 89
	t7: 90
	t8: 91
	t9: 92
	t10: 93
	t11: 94
	t12: 95
	t13: 96
	t14: 97
	t15: 98
	EOT
	test_cmp query_all.exp query_all.out
'
test_expect_success 'powerman -t t13 works' '
	$powerman -h $testaddr -t t13 >query_t13.out &&
	echo "t13: 96" >query_t13.exp &&
	test_cmp query_t13.exp query_t13.out
'
test_expect_success 'powerman -t t[13-15] works' '
	$powerman -h $testaddr -t t[13-15] >query_set.out &&
	cat >query_set.exp <<-EOT &&
	t13: 96
	t14: 97
	t15: 98
	EOT
	test_cmp query_set.exp query_set.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
