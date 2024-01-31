#!/bin/sh

test_description='Check Baytech RPC-3 NC device script'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
baytech=$SHARNESS_BUILD_DIRECTORY/t/simulators/baytech
rpc3dev=$SHARNESS_TEST_SRCDIR/../etc/devices/baytech-rpc3-nc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11013

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman.conf with two rpc3 devices (16 plugs)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$rpc3dev"
	device "b0" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b1" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	node "t[0-7]" "b0"
	node "t[8-15]" "b1"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-15]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t0,t8 works' '
	$powerman -h $testaddr -1 t0,t8 >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows t0,t8 on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0,8]" "t[1-7,9-15]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -c t0,t8 works' '
	$powerman -h $testaddr -c t0,t8 >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows t0,t8 on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0,8]" "t[1-7,9-15]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t0,t8 works' '
	$powerman -h $testaddr -0 t0,t8 >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-15]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-15]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -c t[0-15] works' '
	$powerman -h $testaddr -c t[0-15] >cycle2.out &&
	echo Command completed successfully >cycle2.exp &&
	test_cmp cycle2.exp cycle2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-15]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr -0 t[0-15] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "" "t[0-15]" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_expect_success 'create powerman.conf with 16 rpc3 devices (128 plugs)' '
	cat >powerman2.conf <<-EOT
	listen "$testaddr"
	include "$rpc3dev"
	device "b0" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b1" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b2" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b3" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b4" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b5" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b6" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b7" "baytech-rpc3-nc" "$baytech -p rpc3-nc |&"
	device "b8" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	device "b9" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	device "b10" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	device "b11" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	device "b12" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	device "b13" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	device "b14" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	device "b15" "baytech-rpc3-nc" "$baytech -p rpc3-de |&"
	node "t[0-7]" "b0"
	node "t[8-15]" "b1"
	node "t[16-23]" "b2"
	node "t[24-31]" "b3"
	node "t[32-39]" "b4"
	node "t[40-47]" "b5"
	node "t[48-55]" "b6"
	node "t[56-63]" "b7"
	node "t[64-71]" "b8"
	node "t[72-79]" "b9"
	node "t[80-87]" "b10"
	node "t[88-95]" "b11"
	node "t[96-103]" "b12"
	node "t[104-111]" "b13"
	node "t[112-119]" "b14"
	node "t[120-127]" "b15"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman2.conf -f &
	echo $! >powermand2.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >big_query.out &&
	makeoutput "" "t[0-127]" "" >big_query.exp &&
	test_cmp big_query.exp big_query.out
'

SET1="t[0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120]"
SET2="t[1-7,9-15,17-23,25-31,33-39,41-47,49-55,57-63,65-71,73-79,81-87,89-95,97-103,105-111,113-119,121-127]"

test_expect_success 'powerman -1 first plug in each device works' '
	$powerman -h $testaddr -1 $SET1 >big_on.out &&
	echo Command completed successfully >big_on.exp &&
	test_cmp big_on.exp big_on.out
'
test_expect_success 'powerman -q shows expected output' '
	$powerman -h $testaddr -q >big_query2.out &&
	makeoutput "$SET1" "$SET2" "" >big_query2.exp
	test_cmp big_query2.exp big_query2.out
'
test_expect_success 'powerman -c first plug in each device works' '
	$powerman -h $testaddr -c $SET1 >big_cycle.out &&
	echo Command completed successfully >big_cycle.exp &&
	test_cmp big_cycle.exp big_cycle.out
'
test_expect_success 'powerman -q shows expected output' '
	$powerman -h $testaddr -q >big_query3.out &&
	makeoutput "$SET1" "$SET2" "" >big_query3.exp
	test_cmp big_query3.exp big_query3.out
'
test_expect_success 'powerman -0 first plug in each device works' '
	$powerman -h $testaddr -0 $SET1 >big_off.out &&
	echo Command completed successfully >big_off.exp &&
	test_cmp big_off.exp big_off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >big_query4.out &&
	makeoutput "" "t[0-127]" "" >big_query4.exp &&
	test_cmp big_query4.exp big_query4.out
'
test_expect_success 'powerman -1 t[0-127] works' '
	$powerman -h $testaddr -1 t[0-127] >big_on2.out &&
	echo Command completed successfully >big_on2.exp &&
	test_cmp big_on2.exp big_on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >big_query5.out &&
	makeoutput "t[0-127]" "" "" >big_query5.exp &&
	test_cmp big_query5.exp big_query5.out
'
test_expect_success 'powerman -0 t[0-127] works' '
	$powerman -h $testaddr -0 t[0-127] >big_off2.out &&
	echo Command completed successfully >big_off2.exp &&
	test_cmp big_off2.exp big_off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >big_query6.out &&
	makeoutput "" "t[0-127]" "" >big_query6.exp &&
	test_cmp big_query6.exp big_query6.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand2.pid) &&
	wait
'

test_done

# vi: set ft=sh
