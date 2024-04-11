#!/bin/sh

test_description='Test Powerman diagnostics'

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
	$powerman -h $testaddr -q >queryA1.out &&
	makeoutput "" "t[0-15]" "" >queryA1.exp &&
	test_cmp queryA1.exp queryA1.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >on1.out 2>on1.err &&
	echo Command completed successfully >on1.exp &&
	test_cmp on1.exp on1.out &&
	test_must_be_empty on1.err
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >queryA2.out &&
	makeoutput "t[0-15]" "" "" >queryA2.exp &&
	test_cmp queryA2.exp queryA2.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr -0 t[0-15] >off1.out 2>off1.err &&
	echo Command completed successfully >off1.exp &&
	test_cmp off1.exp off1.out &&
	test_must_be_empty off1.err
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryA3.out &&
	makeoutput "" "t[0-15]" "" >queryA3.exp &&
	test_cmp queryA3.exp queryA3.out
'
test_expect_success 'powerman -1 t[0-7] works' '
	$powerman -h $testaddr -1 t[0-7] >on2.out 2>on2.err &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out &&
	test_must_be_empty on2.err
'
test_expect_success 'powerman -q shows t[0-7] on' '
	$powerman -h $testaddr -q >queryA4.out &&
	makeoutput "t[0-7]" "t[8-15]" "" >queryA4.exp &&
	test_cmp queryA4.exp queryA4.out
'
test_expect_success 'powerman -0 t[0-7] works' '
	$powerman -h $testaddr -0 t[0-7] >off2.out 2>off2.err &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out &&
	test_must_be_empty off2.err
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >queryA5.out &&
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
	$powerman -h $testaddr -q >queryBP1.out &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >queryBP1.exp &&
	test_cmp queryBP1.exp queryBP1.out
'
test_expect_success 'powerman -1 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -1 t[0-15] >onBP1.out 2>onBP1.err &&
	echo Command completed with errors >onBP1.exp &&
	test_cmp onBP1.exp onBP1.out &&
	echo t7: ERROR >onBP1.err_exp &&
	echo t15: ERROR >>onBP1.err_exp &&
	test_cmp onBP1.err_exp onBP1.err
'
test_expect_success 'powerman -q shows t[0-6,8-14] on, t[7,15] unknown' '
	$powerman -h $testaddr -q >queryBP2.out &&
	makeoutput "t[0-6,8-14]" "" "t[7,15]" >>queryBP2.exp &&
	test_cmp queryBP2.exp queryBP2.out
'
test_expect_success 'powerman -0 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[0-15] >offBP1.out 2>offBP1.err &&
	echo Command completed with errors >>offBP1.exp &&
	test_cmp offBP1.exp offBP1.out &&
	echo t7: ERROR >offBP1.err_exp &&
	echo t15: ERROR >>offBP1.err_exp &&
	test_cmp offBP1.err_exp offBP1.err
'
test_expect_success 'powerman -q shows t[0-6,8-14] off, t[7,15] unknown' '
	$powerman -h $testaddr -q >queryBP3.out &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >>queryBP3.exp &&
	test_cmp queryBP3.exp queryBP3.out
'
test_expect_success 'powerman -1 t[0-7] fails' '
	test_must_fail $powerman -h $testaddr -1 t[0-7] >onBP2.out 2>onBP2.err &&
	echo Command completed with errors >>onBP2.exp &&
	test_cmp onBP2.exp onBP2.out &&
	echo t7: ERROR >onBP2.err_exp &&
	test_cmp onBP2.err_exp onBP2.err
'
test_expect_success 'powerman -q shows t[0-6] on' '
	$powerman -h $testaddr -q >queryBP4.out &&
	makeoutput "t[0-6]" "t[8-14]" "t[7,15]" >>queryBP4.exp &&
	test_cmp queryBP4.exp queryBP4.out
'
test_expect_success 'powerman -0 t[0-7] fails' '
	test_must_fail $powerman -h $testaddr -0 t[0-7] >offBP2.out 2>offBP2.err &&
	echo Command completed with errors >>offBP2.exp &&
	test_cmp offBP2.exp offBP2.out &&
	echo t7: ERROR >offBP2.err_exp &&
	test_cmp offBP2.err_exp offBP2.err
'
test_expect_success 'powerman -q shows t[0-6,8-14] off, t[7,15] unknown' '
	$powerman -h $testaddr -q >queryBP5.out &&
	makeoutput "" "t[0-6,8-14]" "t[7,15]" >>queryBP5.exp &&
	test_cmp queryBP5.exp queryBP5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
