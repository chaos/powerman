#!/bin/sh

test_description='Test Powerman power result'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11035

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
	$powerman -h $testaddr -q >query1.out &&
	makeoutput "" "t[0-15]" "" >query1.exp &&
	test_cmp query1.exp query1.out
'
# cover "all" script
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >on1.out &&
	echo Command completed successfully >on1.exp &&
	test_cmp on1.exp on1.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0-15]" "" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr -0 t[0-15] >off1.out &&
	echo Command completed successfully >off1.exp &&
	test_cmp off1.exp off1.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-15]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
# cover "ranged" script
test_expect_success 'powerman -1 t[8-15] works' '
	$powerman -h $testaddr -1 t[8-15] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows t[8-15]  on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[8-15]" "t[0-7]" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -0 t[8-15] works' '
	$powerman -h $testaddr -0 t[8-15] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "" "t[0-15]" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
# cover "singlet" script
test_expect_success 'powerman -1 t15 works' '
	$powerman -h $testaddr -1 t15 >on3.out &&
	echo Command completed successfully >on3.exp &&
	test_cmp on3.exp on3.out
'
test_expect_success 'powerman -q shows t15  on' '
	$powerman -h $testaddr -q >query6.out &&
	makeoutput "t15" "t[0-14]" "" >query6.exp &&
	test_cmp query6.exp query6.out
'
test_expect_success 'powerman -0 t15 works' '
	$powerman -h $testaddr -0 t15 >off3.out &&
	echo Command completed successfully >off3.exp &&
	test_cmp off3.exp off3.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query7.out &&
	makeoutput "" "t[0-15]" "" >query7.exp &&
	test_cmp query7.exp query7.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# plug 15 set to bad, so will not work with anything below
#

test_expect_success 'create test powerman.conf' '
	cat >powerman_bad_plug.conf <<-EOT
	include "$vpcdev"
	listen "$testaddr"
	device "test0" "vpc" "$vpcd --bad-plug=15 |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman_bad_plug.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q  >/dev/null
'
test_expect_success 'powerman -q shows t[0-14] off' '
	$powerman -h $testaddr -q >queryB1.out &&
	makeoutput "" "t[0-14]" "t15" >queryB1.exp &&
	test_cmp queryB1.exp queryB1.out
'
# cover "all" script
test_expect_success 'powerman -1 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -1 t[0-15] >onB1.out &&
	echo Command completed with errors >onB1.exp &&
	test_cmp onB1.exp onB1.out
'
test_expect_success 'powerman -q shows t[0-14] on' '
	$powerman -h $testaddr -q >queryB2.out &&
	makeoutput "t[0-14]" "" "t15" >queryB2.exp &&
	test_cmp queryB2.exp queryB2.out
'
test_expect_success 'powerman -0 t[0-15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[0-15] >offB1.out &&
	echo Command completed with errors >offB1.exp &&
	test_cmp offB1.exp offB1.out
'
test_expect_success 'powerman -q shows t[0-14] off' '
	$powerman -h $testaddr -q >queryB3.out &&
	makeoutput "" "t[0-14]" "t15" >queryB3.exp &&
	test_cmp queryB3.exp queryB3.out
'
# cover "ranged" script
test_expect_success 'powerman -1 t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -1 t[8-15] >onB2.out &&
	echo Command completed with errors >onB2.exp &&
	test_cmp onB2.exp onB2.out
'
test_expect_success 'powerman -q shows t[8-14] on' '
	$powerman -h $testaddr -q >queryB4.out &&
	makeoutput "t[8-14]" "t[0-7]" "t15" >queryB4.exp &&
	test_cmp queryB4.exp queryB4.out
'
test_expect_success 'powerman -0 t[8-15] fails' '
	test_must_fail $powerman -h $testaddr -0 t[8-15] >offB2.out &&
	echo Command completed with errors >offB2.exp &&
	test_cmp offB2.exp offB2.out
'
test_expect_success 'powerman -q shows t[0-14] off' '
	$powerman -h $testaddr -q >queryB5.out &&
	makeoutput "" "t[0-14]" "t15" >queryB5.exp &&
	test_cmp queryB5.exp queryB5.out
'
# cover "singlet" script
test_expect_success 'powerman -1 t15 fails' '
	test_must_fail $powerman -h $testaddr -1 t15 >onB3.out &&
	echo Command completed with errors >onB3.exp &&
	test_cmp onB3.exp onB3.out
'
test_expect_success 'powerman -q shows t[0-14] off' '
	$powerman -h $testaddr -q >queryB6.out &&
	makeoutput "" "t[0-14]" "t15" >queryB6.exp &&
	test_cmp queryB6.exp queryB6.out
'
test_expect_success 'powerman -0 t15 fails' '
	test_must_fail $powerman -h $testaddr -0 t15 >offB3.out &&
	echo Command completed with errors >offB3.exp &&
	test_cmp offB3.exp offB3.out
'
test_expect_success 'powerman -q shows t[0-14] off' '
	$powerman -h $testaddr -q >queryB7.out &&
	makeoutput "" "t[0-14]" "t15" >queryB7.exp &&
	test_cmp queryB7.exp queryB7.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
