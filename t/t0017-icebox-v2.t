#!/bin/sh

test_description='Check Linux Networx icebox v2 device script'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
icebox=$SHARNESS_BUILD_DIRECTORY/t/simulators/icebox
iceboxdev=$SHARNESS_TEST_SRCDIR/../etc/devices/icebox.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11017

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman.conf with one icebox device (10 plugs)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$iceboxdev"
	device "i0" "icebox" "$icebox -p v2 |&"
	node "t[0-9]" "i0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-9]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t0 works' '
	$powerman -h $testaddr -1 t0 >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t0" "t[1-9]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -c t0 works' '
	$powerman -h $testaddr -c t0 >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t0" "t[1-9]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t0 works' '
	$powerman -h $testaddr -0 t0 >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-9]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -1 t[0-9] works' '
	$powerman -h $testaddr -1 t[0-9] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-9]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -c t[0-9] works' '
	$powerman -h $testaddr -c t[0-9] >cycle2.out &&
	echo Command completed successfully >cycle2.exp &&
	test_cmp cycle2.exp cycle2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-9]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -0 t[0-9] works' '
	$powerman -h $testaddr -0 t[0-9] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "" "t[0-9]" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'powerman -t works' '
	$powerman -h $testaddr -t >temp.out &&
	cat >temp.exp <<-EOT &&
	t0: 73,0,0,0
	t1: 74,0,0,0
	t2: 75,0,0,0
	t3: 76,0,0,0
	t4: 77,0,0,0
	t5: 78,0,0,0
	t6: 79,0,0,0
	t7: 80,0,0,0
	t8: 81,0,0,0
	t9: 82,0,0,0
	EOT
	test_cmp temp.exp temp.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_expect_success 'create powerman.conf with 16 icebox devices (160 plugs)' '
	cat >powerman2.conf <<-EOT
	listen "$testaddr"
	include "$iceboxdev"
	device "i0" "icebox" "$icebox -p v2 |&"
	device "i1" "icebox" "$icebox -p v2 |&"
	device "i2" "icebox" "$icebox -p v2 |&"
	device "i3" "icebox" "$icebox -p v2 |&"
	device "i4" "icebox" "$icebox -p v2 |&"
	device "i5" "icebox" "$icebox -p v2 |&"
	device "i6" "icebox" "$icebox -p v2 |&"
	device "i7" "icebox" "$icebox -p v2 |&"
	device "i8" "icebox" "$icebox -p v2 |&"
	device "i9" "icebox" "$icebox -p v2 |&"
	device "i10" "icebox" "$icebox -p v2 |&"
	device "i11" "icebox" "$icebox -p v2 |&"
	device "i12" "icebox" "$icebox -p v2 |&"
	device "i13" "icebox" "$icebox -p v2 |&"
	device "i14" "icebox" "$icebox -p v2 |&"
	device "i15" "icebox" "$icebox -p v2 |&"
	node "t[0-9]"   "i0"
	node "t[10-19]" "i1"
	node "t[20-29]" "i2"
	node "t[30-39]" "i3"
	node "t[40-49]" "i4"
	node "t[50-59]" "i5"
	node "t[60-69]" "i6"
	node "t[70-79]" "i7"
	node "t[80-89]" "i8"
	node "t[90-99]" "i9"
	node "t[100-109]" "i10"
	node "t[110-119]" "i11"
	node "t[120-129]" "i12"
	node "t[130-139]" "i13"
	node "t[140-149]" "i14"
	node "t[150-159]" "i15"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman2.conf -f &
	echo $! >powermand2.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >big_query.out &&
	makeoutput "" "t[0-159]" "" >big_query.exp &&
	test_cmp big_query.exp big_query.out
'

SET1="t[0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150]"
SET2="t[1-9,11-19,21-29,31-39,41-49,51-59,61-69,71-79,81-89,91-99,101-109,111-119,121-129,131-139,141-149,151-159]"

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
test_expect_success 'powerman -t works' '
	$powerman -h $testaddr -t >big_temp.out &&
	for i in $(seq 0 15); do \
	    for j in $(seq 0 9); \
	        do echo "t$(($i*10+$j)): $((73+$j)),0,0,0"; \
	    done; \
	done >big_temp.exp &&
	test_cmp big_temp.exp big_temp.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand2.pid) &&
	wait
'

test_done

# vi: set ft=sh
