#!/bin/sh

test_description='Check Baytech RPC-28 NC device script'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
baytech=$SHARNESS_BUILD_DIRECTORY/t/simulators/baytech
rpc28dev=$SHARNESS_TEST_SRCDIR/../etc/devices/baytech-rpc28-nc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11014

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman config with one device (20 plugs)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$rpc28dev"
	device "b0" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	node "t[0-19]" "b0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-19]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t0 works' '
	$powerman -h $testaddr -1 t0 >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >query1.out &&
	makeoutput "t0" "t[1-19]" "" >query1.exp &&
	test_cmp query1.exp query1.out
'
test_expect_success 'powerman -c t0 works' '
	$powerman -h $testaddr -c t0 >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t0" "t[1-19]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t0 works' '
	$powerman -h $testaddr -0 t0 >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-19]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -1 t[0-19] works' '
	$powerman -h $testaddr -1 t[0-19] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-19]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -c t[0-19] works' '
	$powerman -h $testaddr -c t[0-19] >cycle2.out &&
	echo Command completed successfully >cycle2.exp &&
	test_cmp cycle2.exp cycle2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-19]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -0 t[0-19] works' '
	$powerman -h $testaddr -0 t[0-19] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "" "t[0-19]" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_expect_success 'create powerman config with 8 devices (160 plugs)' '
	cat >powerman2.conf <<-EOT
	listen "$testaddr"
	include "$rpc28dev"
	device "b0" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	device "b1" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	device "b2" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	device "b3" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	device "b4" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	device "b5" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	device "b6" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	device "b7" "baytech-rpc28-nc" "$baytech -p rpc28-nc |&"
	node "t[0-19]" "b0"
	node "t[20-39]" "b1"
	node "t[40-59]" "b2"
	node "t[60-79]" "b3"
	node "t[80-99]" "b4"
	node "t[100-119]" "b5"
	node "t[120-139]" "b6"
	node "t[140-159]" "b7"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman2.conf &
	echo $! >powermand2.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >big_query.out &&
	makeoutput "" "t[0-159]" "" >big_query.exp &&
	test_cmp big_query.exp big_query.out
'

SET1_RPC28="t[0,20,40,60,80,100,120,140]"
SET2_RPC28="t[1-19,21-39,41-59,61-79,81-99,101-119,121-139,141-159]"

test_expect_success 'powerman -1 first plug in each device works' '
	$powerman -h $testaddr -1 $SET1_RPC28 >big_on.out &&
	echo Command completed successfully >big_on.exp &&
	test_cmp big_on.exp big_on.out
'
test_expect_success 'powerman -q shows expected output' '
	$powerman -h $testaddr -q >big_query2.out &&
	makeoutput "$SET1_RPC28" "$SET2_RPC28" "" >big_query2.exp
	test_cmp big_query2.exp big_query2.out
'
test_expect_success 'powerman -c first plug in each device works' '
	$powerman -h $testaddr -c $SET1_RPC28 >big_cycle.out &&
	echo Command completed successfully >big_cycle.exp &&
	test_cmp big_cycle.exp big_cycle.out
'
test_expect_success 'powerman -q shows expected output' '
	$powerman -h $testaddr -q >big_query3.out &&
	makeoutput "$SET1_RPC28" "$SET2_RPC28" "" >big_query3.exp
	test_cmp big_query3.exp big_query3.out
'
test_expect_success 'powerman -0 first plug in each device works' '
	$powerman -h $testaddr -0 $SET1_RPC28 >big_off.out &&
	echo Command completed successfully >big_off.exp &&
	test_cmp big_off.exp big_off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >big_query4.out &&
	makeoutput "" "t[0-159]" "" >big_query4.exp &&
	test_cmp big_query4.exp big_query4.out
'
test_expect_success 'powerman -1 t[0-159] works' '
	$powerman -h $testaddr -1 t[0-159] >big_on2.out &&
	echo Command completed successfully >big_on2.exp &&
	test_cmp big_on2.exp big_on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >big_query5.out &&
	makeoutput "t[0-159]" "" "" >big_query5.exp &&
	test_cmp big_query5.exp big_query5.out
'
test_expect_success 'powerman -0 t[0-159] works' '
	$powerman -h $testaddr -0 t[0-159] >big_off2.out &&
	echo Command completed successfully >big_off2.exp &&
	test_cmp big_off2.exp big_off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >big_query6.out &&
	makeoutput "" "t[0-159]" "" >big_query6.exp &&
	test_cmp big_query6.exp big_query6.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand2.pid) &&
	wait
'

test_done

# vi: set ft=sh
