#!/bin/sh

test_description='Test simple Powerman API client'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
test_apiclient=$SHARNESS_BUILD_DIRECTORY/src/powerman/test_apiclient
vpcd=$SHARNESS_BUILD_DIRECTORY/test/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/../etc/devices/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11003

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

test_expect_success 'API query works' '
	$test_apiclient $testaddr q t1 >query.out &&
	cat >query.exp <<-EOT &&
	t1: off
	EOT
	test_cmp query.exp query.out
'
test_expect_success 'API target list works' '
	$test_apiclient $testaddr l >list.out &&
	cat >list.exp <<-EOT &&
	t0
	t1
	t2
	t3
	t4
	t5
	t6
	t7
	t8
	t9
	t10
	t11
	t12
	t13
	t14
	t15
	EOT
	test_cmp list.exp list.out
'
test_expect_success 'API query shows t0 is off' '
	$test_apiclient $testaddr q t0 >query2.out &&
	cat >query2.exp <<-EOT &&
	t0: off
	EOT
	test_cmp query2.exp query2.out
'
test_expect_success 'API can turn on t0' '
	$test_apiclient $testaddr 1 t0
'
test_expect_success 'API query shows t0 is on' '
	$test_apiclient $testaddr q t0 >query3.out &&
	cat >query3.exp <<-EOT &&
	t0: on
	EOT
	test_cmp query3.exp query3.out
'
test_expect_success 'API can turn off t0' '
	$test_apiclient $testaddr 0 t0
'
test_expect_success 'API query shows t0 is off' '
	$test_apiclient $testaddr q t0 >query4.out &&
	cat >query4.exp <<-EOT &&
	t0: off
	EOT
	test_cmp query4.exp query4.out
'

test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
