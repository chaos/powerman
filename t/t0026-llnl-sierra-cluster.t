#!/bin/sh

test_description='Check LLNL sierra cluster config with IPMI + PDU'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
swpdu=$SHARNESS_BUILD_DIRECTORY/test/swpdu
ipmipower=$SHARNESS_BUILD_DIRECTORY/test/ipmipower
ipmipowerdev=$SHARNESS_TEST_SRCDIR/../etc/devices/ipmipower.dev
swpdudev=$SHARNESS_TEST_SRCDIR/../etc/devices/swpdu.dev
plugconf=$SHARNESS_TEST_SRCDIR/etc/sierra_plugs.conf

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11026


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

# This config was adapted from the actual sierra config, where chassis
# had PDU power control and blades had IPMI power control.
test_expect_success 'create powerman.conf for sierra' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$ipmipowerdev"
	include "$swpdudev"
	device "ipmi0" "ipmipower" "$ipmipower -h psierra[0,2-143] |&"
	device "ipmi1" "ipmipower" "$ipmipower -h psierra[144-287] |&"
	device "ipmi2" "ipmipower" "$ipmipower -h psierra[288-431] |&"
	device "ipmi3" "ipmipower" "$ipmipower -h psierra[432-575] |&"
	device "ipmi4" "ipmipower" "$ipmipower -h psierra[576-719] |&"
	device "ipmi5" "ipmipower" "$ipmipower -h psierra[720-863] |&"
	device "ipmi6" "ipmipower" "$ipmipower -h psierra[864-1007] |&"
	device "ipmi7" "ipmipower" "$ipmipower -h psierra[1008-1151] |&"
	device "ipmi8" "ipmipower" "$ipmipower -h psierra[1152-1295] |&"
	device "ipmi9" "ipmipower" "$ipmipower -h psierra[1296-1439] |&"
	device "ipmi10" "ipmipower" "$ipmipower -h psierra[1440-1583] |&"
	device "ipmi11" "ipmipower" "$ipmipower -h psierra[1584-1727] |&"
	device "ipmi12" "ipmipower" "$ipmipower -h psierra[1728-1871] |&"
	device "ipmi13" "ipmipower" "$ipmipower -h psierra[1872-1943] |&"
	device "pdu1" "swpdu"   "$swpdu |&"
	device "pdu2" "swpdu"   "$swpdu |&"
	device "pdu3" "swpdu"   "$swpdu |&"
	device "pdu4" "swpdu"   "$swpdu |&"
	device "pdu5" "swpdu"   "$swpdu |&"
	device "pdu6" "swpdu"   "$swpdu |&"
	device "pdu7" "swpdu"   "$swpdu |&"
	device "pdu8" "swpdu"   "$swpdu |&"
	device "pdu9" "swpdu"   "$swpdu |&"
	device "pdu10" "swpdu"  "$swpdu |&"
	device "pdu11" "swpdu"  "$swpdu |&"
	device "pdu12" "swpdu"  "$swpdu |&"
	device "pdu13" "swpdu"  "$swpdu |&"
	device "pdu14" "swpdu"  "$swpdu |&"
	device "pdu15" "swpdu"  "$swpdu |&"
	#device "pdu16" "swpdu" "$swpdu |&"
	#device "pdu17" "swpdu" "$swpdu |&"
	#device "pdu18" "swpdu" "$swpdu |&"
	device "pdu19" "swpdu"  "$swpdu |&"
	device "pdu20" "swpdu"  "$swpdu |&"
	device "pdu21" "swpdu"  "$swpdu |&"
	device "pdu22" "swpdu"  "$swpdu |&"
	device "pdu23" "swpdu"  "$swpdu |&"
	device "pdu24" "swpdu"  "$swpdu |&"
	device "pdu25" "swpdu"  "$swpdu |&"
	device "pdu26" "swpdu"  "$swpdu |&"
	device "pdu27" "swpdu"  "$swpdu |&"
	device "pdu28" "swpdu"  "$swpdu |&"
	device "pdu29" "swpdu"  "$swpdu |&"
	device "pdu30" "swpdu"  "$swpdu |&"
	device "pdu31" "swpdu"  "$swpdu |&"
	device "pdu32" "swpdu"  "$swpdu |&"
	device "pdu33" "swpdu"  "$swpdu |&"
	include "$plugconf"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f >powermand.out 2>powermand.err &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d >device.out
'
test_expect_success 'powerman -Q sierra[0,2-1943] shows all blades off' '
	$powerman -h $testaddr -Q sierra[0,2-1943] >query_blades.out &&
	makeoutput "" "sierra[0,2-1943]" "" >query_blades.exp &&
	test_cmp query_blades.exp query_blades.out
'
# Messy off: line so just verify that "unknown" and "on" are empty
test_expect_success 'powerman -Q chassis[1-468] shows all chassis off' '
	$powerman -h $testaddr -Q chassis[1-468] >query_chassis.out &&
	grep -q "unknown: $" query_chassis.out &&
	grep -q "on:      $" query_chassis.out
'
test_expect_success 'powerman -1 chassis[1-468] works' '
	$powerman -h $testaddr -1 chassis[1-468] >on_chassis.out &&
	echo Command completed successfully >on_chassis.exp &&
	test_cmp on_chassis.exp on_chassis.out
'
test_expect_success 'powerman -Q chassis[1-468] shows all chassis on' '
	$powerman -h $testaddr -Q chassis[1-468] >query_chassis2.out &&
	grep -q "unknown: $" query_chassis2.out &&
	grep -q "off:     $" query_chassis2.out
'
test_expect_success 'powerman -1 sierra[0,2-1943] works' '
	$powerman -h $testaddr -1 sierra[0,2-1943] >on_blades.out &&
	echo Command completed successfully >on_blades.exp &&
	test_cmp on_blades.exp on_blades.out
'
test_expect_success 'powerman -Q sierra[0,2-1943] shows all blades on' '
	$powerman -h $testaddr -Q sierra[0,2-1943] >query_blades2.out &&
	makeoutput "sierra[0,2-1943]" "" "" >query_blades2.exp &&
	test_cmp query_blades2.exp query_blades2.out
'
test_expect_success 'powerman -0 sierra[0,2-1943] works' '
	$powerman -h $testaddr -0 sierra[0,2-1943] >off_blades.out &&
	echo Command completed successfully >off_blades.exp &&
	test_cmp off_blades.exp off_blades.out
'
test_expect_success 'powerman -Q sierra[0,2-1943] shows all blades off' '
	$powerman -h $testaddr -Q sierra[0,2-1943] >query_blades3.out &&
	makeoutput "" "sierra[0,2-1943]" "" >query_blades3.exp &&
	test_cmp query_blades3.exp query_blades3.out
'
test_expect_success 'powerman -0 chassis[1-468] works' '
	$powerman -h $testaddr -0 chassis[1-468] >off_chassis.out &&
	echo Command completed successfully >off_chassis.exp &&
	test_cmp off_chassis.exp off_chassis.out
'
test_expect_success 'powerman -Q chassis[1-468] shows all chassis off' '
	$powerman -h $testaddr -Q chassis[1-468] >query_chassis3.out &&
	grep -q "unknown: $" query_chassis3.out &&
	grep -q "on:      $" query_chassis3.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
