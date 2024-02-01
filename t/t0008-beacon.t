#!/bin/sh

test_description='Test Powerman beacon options'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11008

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
	$powermand -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -b shows all beacons off' '
	$powerman -h $testaddr -b >query1.out &&
	makeoutput "" "t[0-15]" "" >query1.exp &&
	test_cmp query1.exp query1.out
'
test_expect_success 'powerman -f t[13-15] works' '
	$powerman -h $testaddr -f t[13-15] >flash.out &&
	echo "Command completed successfully" >flash.exp &&
	test_cmp flash.exp flash.out
'
test_expect_success 'powerman -b shows beacons t[13-15] on' '
	$powerman -h $testaddr -b >query2.out &&
	makeoutput "t[13-15]" "t[0-12]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -u t13 works' '
	$powerman -h $testaddr -u t13 >unflash.out &&
	echo "Command completed successfully" >unflash.exp &&
	test_cmp unflash.exp unflash.out
'
test_expect_success 'powerman -b shows beacons t[14-15] on' '
	$powerman -h $testaddr -b >query3.out &&
	makeoutput "t[14-15]" "t[0-13]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -b t13 shows beacon t13 off' '
	$powerman -h $testaddr -b t13 >query4.out &&
	makeoutput "" "t13" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -b t14 shows beacon t14 on' '
	$powerman -h $testaddr -b t14 >query5.out &&
	makeoutput "t14" "" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'powerman -f with no targets fails with useful error' '
	test_must_fail $powerman -h $testaddr -f 2>notargetsf.err &&
	grep "Command requires targets" notargetsf.err
'
test_expect_success 'powerman -u with no targets fails with useful error' '
	test_must_fail $powerman -h $testaddr -u 2>notargetsu.err &&
	grep "Command requires targets" notargetsu.err
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
