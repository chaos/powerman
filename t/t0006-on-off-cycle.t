#!/bin/sh

test_description='Test Powerman power control options'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11006

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

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
test_expect_success 'powerman -q shows all nodes off' '
	$powerman -h $testaddr -q >query1.out &&
	makeoutput "" "t[0-15]" "" >query1.exp &&
	test_cmp query1.exp query1.out
'
test_expect_success 'powerman -1 t4 works' '
	$powerman -h $testaddr -1 t4 >on4.out &&
	echo "Command completed successfully" >on4.exp &&
	test_cmp on4.exp on4.out
'
test_expect_success 'powerman -q shows t4 is on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t4" "t[0-3,5-15]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t4 works' '
	$powerman -h $testaddr -0 t4 >off4.out &&
	echo "Command completed successfully" >off4.exp &&
	test_cmp off4.exp off4.out
'
test_expect_success 'powerman -q shows all nodes off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-15]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -c t5 works' '
	$powerman -h $testaddr -c t5 >cycle5.out &&
	echo "Command completed successfully" >cycle5.exp &&
	test_cmp cycle5.exp cycle5.out
'
test_expect_success 'powerman -q shows t5 on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t5" "t[0-4,6-15]" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -1 t14 t15 works' '
	$powerman -h $testaddr -1 t14 t15 >on5.out &&
	echo "Command completed successfully" >on5.exp
'
test_expect_success 'powerman -q shows t14 t15 on' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "t[5,14-15]" "t[0-4,6-13]" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'powerman -1 with no targets fails with useful error' '
	test_must_fail $powerman -h $testaddr -1 2>notargets1.err &&
	grep "Command requires targets" notargets1.err
'
test_expect_success 'powerman -0 with no targets fails with useful error' '
	test_must_fail $powerman -h $testaddr -0 2>notargets0.err &&
	grep "Command requires targets" notargets0.err
'
test_expect_success 'powerman -c with no targets fails with useful error' '
	test_must_fail $powerman -h $testaddr -c 2>notargetsc.err &&
	grep "Command requires targets" notargetsc.err
'

test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
