#!/bin/sh

test_description='Test Powerman diag'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11036

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
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr --diag -q >queryA1.out &&
	makeoutput "" "t[0-15]" "" >queryA1.exp &&
	test_cmp queryA1.exp queryA1.out
'
test_expect_success 'powerman --diag -1 t[0-15] works' '
	$powerman -h $testaddr --diag -1 t[0-15] >onA1.out &&
	echo Command completed successfully >onA1.exp &&
	test_cmp onA1.exp onA1.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr --diag -q >queryA2.out &&
	makeoutput "t[0-15]" "" "" >queryA2.exp &&
	test_cmp queryA2.exp queryA2.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr --diag -0 t[0-15] >offA1.out &&
	echo Command completed successfully >offA1.exp &&
	test_cmp offA1.exp offA1.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr --diag -q >queryA3.out &&
	makeoutput "" "t[0-15]" "" >queryA3.exp &&
	test_cmp queryA3.exp queryA3.out
'
test_expect_success 'powerman --diag -1 t[0-7] works' '
	$powerman -h $testaddr --diag -1 t[0-7] >onA2.out &&
	echo Command completed successfully >onA2.exp &&
	test_cmp onA2.exp onA2.out
'
test_expect_success 'powerman -q shows t[0-7] on' '
	$powerman -h $testaddr --diag -q >queryA4.out &&
	makeoutput "t[0-7]" "t[8-15]" "" >queryA4.exp &&
	test_cmp queryA4.exp queryA4.out
'
test_expect_success 'powerman -0 t[0-7] works' '
	$powerman -h $testaddr --diag -0 t[0-7] >offA2.out &&
	echo Command completed successfully >offA2.exp &&
	test_cmp offA2.exp offA2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr --diag -q >queryA5.out &&
	makeoutput "" "t[0-15]" "" >queryA5.exp &&
	test_cmp queryA5.exp queryA5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# t7,t15 set to bad, so will not work with anything below
#

test_expect_success 'create test powerman.conf' '
	cat >powerman_bad_plug.conf <<-EOT
	include "$vpcdev"
	listen "$testaddr"
	device "test0" "vpc" "$vpcd --bad-plug=7 --bad-plug=15 |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman_bad_plug.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q shows t[0-6,8-14] off, t[7,15] unknown' '
	$powerman -h $testaddr --diag -q >queryBP1.out &&
	echo t[7,15]: ERROR >queryBP1.exp &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >>queryBP1.exp &&
	test_cmp queryBP1.exp queryBP1.out
'
test_expect_success 'powerman --diag -1 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr --diag -1 t[0-15] >onBP1.out &&
	echo t[7,15]: ERROR >onBP1.exp &&
	echo Command completed with errors >>onBP1.exp &&
	test_cmp onBP1.exp onBP1.out
'
test_expect_success 'powerman -q shows t[0-6,8-14] on, t[7,15] unknown' '
	$powerman -h $testaddr --diag -q >queryBP2.out &&
	echo t[7,15]: ERROR >queryBP2.exp &&
	makeoutput "t[0-6,8-14]" "" "t[7,15]" >>queryBP2.exp &&
	test_cmp queryBP2.exp queryBP2.out
'
test_expect_success 'powerman -0 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr --diag -0 t[0-15] >offBP1.out &&
	echo t[7,15]: ERROR >offBP1.exp &&
	echo Command completed with errors >>offBP1.exp &&
	test_cmp offBP1.exp offBP1.out
'
test_expect_success 'powerman -q shows t[0-6,8-14] off, t[7,15] unknown' '
	$powerman -h $testaddr --diag -q >queryBP3.out &&
	echo t[7,15]: ERROR >queryBP3.exp &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >>queryBP3.exp &&
	test_cmp queryBP3.exp queryBP3.out
'
test_expect_success 'powerman --diag -1 t[0-7] fails' '
	test_must_fail $powerman -h $testaddr --diag -1 t[0-7] >onBP2.out &&
	echo t7: ERROR >onBP2.exp &&
	echo Command completed with errors >>onBP2.exp &&
	test_cmp onBP2.exp onBP2.out
'
test_expect_success 'powerman -q shows t[0-6] on' '
	$powerman -h $testaddr --diag -q >queryBP4.out &&
	echo t[7,15]: ERROR >queryBP4.exp &&
	makeoutput "t[0-6]" "t[8-14]" "t[7,15]" >>queryBP4.exp &&
	test_cmp queryBP4.exp queryBP4.out
'
test_expect_success 'powerman -0 t[0-7] fails' '
	test_must_fail $powerman -h $testaddr --diag -0 t[0-7] >offBP2.out &&
	echo t7: ERROR >offBP2.exp &&
	echo Command completed with errors >>offBP2.exp &&
	test_cmp offBP2.exp offBP2.out
'
test_expect_success 'powerman -q shows t[0-6,8-14] off, t[7,15] unknown' '
	$powerman -h $testaddr --diag -q >queryBP5.out &&
	echo t[7,15]: ERROR >queryBP5.exp &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >>queryBP5.exp &&
	test_cmp queryBP5.exp queryBP5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
