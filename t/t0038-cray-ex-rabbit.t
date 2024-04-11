#!/bin/sh

test_description='Test Cray EX chassis with redfish'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
redfishdir=$SHARNESS_BUILD_DIRECTORY/src/redfishpower
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11038


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

#
# redfishpower hpe cray supercomputing ex chassis test - rabbit
#
# note rabbit blade is descendant of perif 7
#

test_expect_success 'create powerman.conf for chassis w/ 16 redfish nodes and rabbit (crayexR)' '
	cat >powerman_cray_ex_R.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-cray-ex-rabbit.dev"
	device "d0" "cray-ex-rabbit" "$redfishdir/redfishpower -h cmm0,t[0-15],rabbit --test-mode |&"
	node "cmm0,perif[0-4,7],blade[0-7],t[0-15],rabbit" "d0" "Enclosure,Perif[0-4,7],Blade[0-7],Node[0-16]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (crayexR)' '
	$powermand -Y -c powerman_cray_ex_R.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayexR_query1.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-4,7],rabbit,t[0-15]" "" >test_crayexR_query1.exp &&
	test_cmp test_crayexR_query1.exp test_crayexR_query1.out
'
# powering on rabbit with cmm and perif7 off
test_expect_success 'powerman -1 rabbit doesnt work' '
	test_must_fail $powerman -h $testaddr -1 rabbit >test_crayexR_on1.out 2>test_crayexR_on1.err &&
	echo Command completed with errors >>test_crayexR_on1.exp &&
	test_cmp test_crayexR_on1.exp test_crayexR_on1.out &&
	grep "rabbit: cannot perform on, dependency off" test_crayexR_on1.err
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayexR_query2.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-4,7],rabbit,t[0-15]" "" >test_crayexR_query2.exp &&
	test_cmp test_crayexR_query2.exp test_crayexR_query2.out
'
# turn on cmm works, can't turn on rabbit
test_expect_success 'powerman -1 cmm0 completes' '
	$powerman -h $testaddr -1 cmm0 >test_crayexR_on2.out &&
	echo Command completed successfully >test_crayexR_on2.exp &&
	test_cmp test_crayexR_on2.exp test_crayexR_on2.out
'
test_expect_success 'powerman -1 rabbit doesnt work' '
	test_must_fail $powerman -h $testaddr -1 rabbit >test_crayexR_on3.out 2>test_crayexR_on3.err &&
	echo Command completed with errors >>test_crayexR_on3.exp &&
	test_cmp test_crayexR_on3.exp test_crayexR_on3.out &&
	grep "rabbit: cannot perform on, dependency off" test_crayexR_on3.err
'
test_expect_success 'powerman -q shows cmm0 on' '
	$powerman -h $testaddr -q >test_crayexR_query3.out &&
	makeoutput "cmm0" "blade[0-7],perif[0-4,7],rabbit,t[0-15]" "" >test_crayexR_query3.exp &&
	test_cmp test_crayexR_query3.exp test_crayexR_query3.out
'
# turn on perif7 works, can turn on rabbit now
test_expect_success 'powerman -1 perif7 completes' '
	$powerman -h $testaddr -1 perif7 >test_crayexR_on4.out &&
	echo Command completed successfully >test_crayexR_on4.exp &&
	test_cmp test_crayexR_on4.exp test_crayexR_on4.out
'
test_expect_success 'powerman -1 rabbit completes' '
	$powerman -h $testaddr -1 rabbit >test_crayexR_on5.out &&
	echo Command completed successfully >test_crayexR_on5.exp &&
	test_cmp test_crayexR_on5.exp test_crayexR_on5.out
'
test_expect_success 'powerman -q shows cmm0,perif7,rabbit on' '
	$powerman -h $testaddr -q >test_crayexR_query4.out &&
	makeoutput "cmm0,perif7,rabbit" "blade[0-7],perif[0-4],t[0-15]" "" >test_crayexR_query4.exp &&
	test_cmp test_crayexR_query4.exp test_crayexR_query4.out
'
# turn off rabbit, perif7, cmm0
test_expect_success 'powerman -0 rabbit completes' '
	$powerman -h $testaddr -0 rabbit >test_crayexR_off1.out &&
	echo Command completed successfully >test_crayexR_off1.exp &&
	test_cmp test_crayexR_off1.exp test_crayexR_off1.out
'
test_expect_success 'powerman -0 perif7 completes' '
	$powerman -h $testaddr -0 perif7 >test_crayexR_off2.out &&
	echo Command completed successfully >test_crayexR_off2.exp &&
	test_cmp test_crayexR_off2.exp test_crayexR_off2.out
'
test_expect_success 'powerman -0 cmm0 completes' '
	$powerman -h $testaddr -0 cmm0 >test_crayexR_off3.out &&
	echo Command completed successfully >test_crayexR_off3.exp &&
	test_cmp test_crayexR_off3.exp test_crayexR_off3.out
'
test_expect_success 'powerman -q shows everything off' '
	$powerman -h $testaddr -q >test_crayexR_query5.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-4,7],rabbit,t[0-15]" "" >test_crayexR_query5.exp &&
	test_cmp test_crayexR_query5.exp test_crayexR_query5.out
'
# turn on everything
test_expect_success 'powerman turn on everything' '
	$powerman -h $testaddr -1 cmm0 >test_crayexR_on6A.out &&
	echo Command completed successfully >test_crayexR_on6A.exp &&
	test_cmp test_crayexR_on6A.exp test_crayexR_on6A.out &&
	$powerman -h $testaddr -1 blade[0-7],perif[0-4,7] >test_crayexR_on6B.out &&
	echo Command completed successfully >test_crayexR_on6B.exp &&
	test_cmp test_crayexR_on6B.exp test_crayexR_on6B.out &&
	$powerman -h $testaddr -1 t[0-15],rabbit >test_crayexR_on6C.out &&
	echo Command completed successfully >test_crayexR_on6C.exp &&
	test_cmp test_crayexR_on6C.exp test_crayexR_on6C.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_crayexR_query6.out &&
	makeoutput "blade[0-7],cmm0,perif[0-4,7],rabbit,t[0-15]" "" "" >test_crayexR_query6.exp &&
	test_cmp test_crayexR_query6.exp test_crayexR_query6.out
'
# turn off everything works
test_expect_success 'powerman -0 blade[0-7],cmm0,perif[0-4,7],rabbit,t[0-15] completes' '
	$powerman -h $testaddr -0 blade[0-7],cmm0,perif[0-4,7],rabbit,t[0-15] >test_crayexR_off4.out &&
	echo Command completed successfully >test_crayexR_off4.exp &&
	test_cmp test_crayexR_off4.exp test_crayexR_off4.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayexR_query7.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-4,7],rabbit,t[0-15]" "" >test_crayexR_query7.exp &&
	test_cmp test_crayexR_query7.exp test_crayexR_query7.out
'
test_expect_success 'stop powerman daemon (crayexR)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
