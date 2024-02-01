#!/bin/sh

test_description='Check powermand as a power control device'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev
powermandev=$SHARNESS_TEST_SRCDIR/../etc/devices/powerman.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11021


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create controlled powerman.conf with 4 devices (64 plugs)' '
	cat >rpowerman.conf <<-EOT
	include "$vpcdev"
	device "test0" "vpc" "$vpcd |&"
	device "test1" "vpc" "$vpcd |&"
	device "test2" "vpc" "$vpcd |&"
	device "test3" "vpc" "$vpcd |&"
	node "t[0-15]"  "test0"
	node "t[16-31]" "test1"
	node "t[32-47]" "test2"
	node "t[48-63]" "test3"
	EOT
'
test_expect_success 'create controlling powerman.conf' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$powermandev"
	device "p0" "powerman" "$powermand --stdio -c rpowerman.conf |&"
	node "t[0-63]"   "p0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'

SET1="t[0,8,16,24,32,40,48,56]"
SET2="t[1-7,9-15,17-23,25-31,33-39,41-47,49-55,57-63]"

test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-63]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success "powerman -1 $SET1 works" '
	$powerman -h $testaddr -1 $SET1 >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows expected result' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "$SET1" "$SET2" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success "powerman -c $SET1 works" '
	$powerman -h $testaddr -c $SET1 >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows expected result' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "$SET1" "$SET2" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success "powerman -0 $SET1 works" '
	$powerman -h $testaddr -0 $SET1 >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-63]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success "powerman -r $SET1 works" '
	$powerman -h $testaddr -r $SET1 >reset.out &&
	echo Command completed successfully >reset.exp &&
	test_cmp reset.exp reset.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "" "t[0-63]" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -1 t[0-63] works' '
	$powerman -h $testaddr -1 t[0-63] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "t[0-63]" "" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'powerman -c t[0-63] works' '
	$powerman -h $testaddr -c t[0-63] >cycle2.out &&
	echo Command completed successfully >cycle2.exp &&
	test_cmp cycle2.exp cycle2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query6.out &&
	makeoutput "t[0-63]" "" "" >query6.exp &&
	test_cmp query6.exp query6.out
'
test_expect_success 'powerman -0 t[0-63] works' '
	$powerman -h $testaddr -0 t[0-63] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query7.out &&
	makeoutput "" "t[0-63]" "" >query7.exp &&
	test_cmp query7.exp query7.out
'
test_expect_success 'powerman -r t[0-63] works' '
	$powerman -h $testaddr -r t[0-63] >reset2.out &&
	echo Command completed successfully >reset2.exp &&
	test_cmp reset2.exp reset2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query8.out &&
	makeoutput "" "t[0-63]" "" >query8.exp &&
	test_cmp query8.exp query8.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
