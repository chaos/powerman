#!/bin/sh

test_description='Check LLNL MCR config with many icebox v3 devices'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
simdir=$SHARNESS_BUILD_DIRECTORY/t/simulators
devicedir=$SHARNESS_TEST_SRCDIR/../etc/devices
plugconf=$SHARNESS_TEST_SRCDIR/etc/mcr_plugs.conf

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11031

echo "Command completed successfully" >success.exp

is_successful() {
	test_cmp success.exp $1
}

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

# This config was adapted from the actual MCR config.  This machine was a
# node count scaling milestone for early Linux clusters at LLNL and thus
# became a sandbox during pre-production integration for improving the lab's
# Linux system system, including powerman.
test_expect_success 'create powerman.conf for MCR' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$devicedir/icebox3.dev"
	device "ice-r1-1"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r2-1"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r4-1"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r5-1"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r6-1"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r7-1"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r7-2"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r7-3"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r8-1"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r8-2"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r8-3"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r9-1"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r9-2"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r9-3"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r9-4"  "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r10-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r10-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r11-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r11-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r11-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r11-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r15-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r15-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r15-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r15-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r16-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r16-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r17-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r17-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r17-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r17-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r18-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r18-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r18-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r18-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r19-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r19-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r20-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r20-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r20-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r20-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r23-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r23-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r23-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r23-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r24-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r24-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r25-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r25-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r25-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r25-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r26-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r26-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r26-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r26-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r27-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r27-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r28-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r28-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r28-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r28-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r29-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r29-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r29-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r29-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r30-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r30-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r31-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r31-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r31-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r31-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r32-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r32-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r32-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r32-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r33-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r33-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r34-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r34-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r34-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r34-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r35-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r35-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r35-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r35-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r36-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r36-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r37-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r37-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r37-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r37-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r38-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r38-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r38-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r38-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r39-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r39-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r40-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r40-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r40-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r40-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r41-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r41-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r41-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r41-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r42-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r42-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r43-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r43-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r43-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r43-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r12-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r12-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r12-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r12-4" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r13-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r13-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r14-1" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r14-2" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r14-3" "icebox3" "$simdir/icebox -pv3|&"
	device "ice-r14-4" "icebox3" "$simdir/icebox -pv3|&"
	include "$plugconf"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f >powermand.out 2>powermand.err &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d >device.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "mcr[0-1151],mcrj" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 mcr[0-1151],mcrj works' '
	$powerman -h $testaddr -1 mcr[0-1151],mcrj >on.out &&
	is_successful on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "mcr[0-1151],mcrj" "" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 mcr[0-1151],mcrj works' '
	$powerman -h $testaddr -0 mcr[0-1151],mcrj >off.out &&
	is_successful off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "mcr[0-1151],mcrj" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
