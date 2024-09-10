#!/bin/sh

test_description='Test Cray EX chassis with redfish'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
redfishdir=$SHARNESS_BUILD_DIRECTORY/src/redfishpower
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11037


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

#
# redfishpower hpe cray supercomputing ex chassis basic test
#

test_expect_success 'create powerman.conf for chassis w/ 16 redfish nodes (crayex)' '
	cat >powerman_cray_ex.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-cray-ex.dev"
	device "d0" "cray-ex" "$redfishdir/redfishpower -h cmm0,t[0-15] --test-mode |&"
	node "cmm0,perif[0-7],blade[0-7],t[0-15]" "d0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (crayex)' '
	$powermand -Y -c powerman_cray_ex.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayex_query1.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-7],t[0-15]" "" >test_crayex_query1.exp &&
	test_cmp test_crayex_query1.exp test_crayex_query1.out
'
# powering on descendants with cmm off, nothing turns on
test_expect_success 'powerman -1 blade[0-7] doesnt work' '
	test_must_fail $powerman -h $testaddr -1 blade[0-7] >test_crayex_on1.out 2>test_crayex_on1.err &&
	echo Command completed with errors >>test_crayex_on1.exp &&
	test_cmp test_crayex_on1.exp test_crayex_on1.out &&
	count=`grep ^blade test_crayex_on1.err | wc -l` &&
	test $count -eq 8 &&
	count=`grep "cannot perform on, dependency off" test_crayex_on1.err | wc -l` &&
	test $count -eq 8
'
test_expect_success 'powerman -1 perif[0-7] doesnt work' '
	test_must_fail $powerman -h $testaddr -1 perif[0-7] >test_crayex_on2.out 2>test_crayex_on2.err &&
	echo Command completed with errors >>test_crayex_on2.exp &&
	test_cmp test_crayex_on2.exp test_crayex_on2.out &&
	count=`grep ^perif test_crayex_on2.err | wc -l` &&
	test $count -eq 8 &&
	count=`grep "cannot perform on, dependency off" test_crayex_on2.err | wc -l` &&
	test $count -eq 8
'
test_expect_success 'powerman -1 t[0-15] doesnt work' '
	test_must_fail $powerman -h $testaddr -1 t[0-15] >test_crayex_on3.out 2>test_crayex_on3.err &&
	echo Command completed with errors >>test_crayex_on3.exp &&
	test_cmp test_crayex_on3.exp test_crayex_on3.out &&
	count=`grep ^t test_crayex_on3.err | wc -l` &&
	test $count -eq 16 &&
	count=`grep "cannot perform on, dependency off" test_crayex_on3.err | wc -l` &&
	test $count -eq 16
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayex_query2.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-7],t[0-15]" "" >test_crayex_query2.exp &&
	test_cmp test_crayex_query2.exp test_crayex_query2.out
'
# turn on cmm works
test_expect_success 'powerman -1 cmm0 completes' '
	$powerman -h $testaddr -1 cmm0 >test_crayex_on4.out &&
	echo Command completed successfully >test_crayex_on4.exp &&
	test_cmp test_crayex_on4.exp test_crayex_on4.out
'
test_expect_success 'powerman -q shows cmm0 on' '
	$powerman -h $testaddr -q >test_crayex_query3.out &&
	makeoutput "cmm0" "blade[0-7],perif[0-7],t[0-15]" "" >test_crayex_query3.exp &&
	test_cmp test_crayex_query3.exp test_crayex_query3.out
'
# turn on children of blades does not work
test_expect_success 'powerman -1 t[0-15] doesnt work' '
	test_must_fail $powerman -h $testaddr -1 t[0-15] >test_crayex_on5.out 2>test_crayex_on5.err &&
	echo Command completed with errors >>test_crayex_on5.exp &&
	test_cmp test_crayex_on5.exp test_crayex_on5.out &&
	count=`grep ^t test_crayex_on5.err | wc -l` &&
	test $count -eq 16 &&
	count=`grep "cannot perform on, dependency off" test_crayex_on5.err | wc -l` &&
	test $count -eq 16
'
test_expect_success 'powerman -q shows cmm0 on' '
	$powerman -h $testaddr -q >test_crayex_query4.out &&
	makeoutput "cmm0" "blade[0-7],perif[0-7],t[0-15]" "" >test_crayex_query4.exp &&
	test_cmp test_crayex_query4.exp test_crayex_query4.out
'
# turn on some blades, allows some nodes to be turned on
test_expect_success 'powerman -1 blade[4-7] completes' '
	$powerman -h $testaddr -1 blade[4-7] >test_crayex_on6.out &&
	echo Command completed successfully >test_crayex_on6.exp &&
	test_cmp test_crayex_on6.exp test_crayex_on6.out
'
test_expect_success 'powerman -1 t[0-15] doesnt work, some succeed' '
	test_must_fail $powerman -h $testaddr -1 t[0-15] >test_crayex_on7.out 2>test_crayex_on7.err &&
	echo Command completed with errors >>test_crayex_on7.exp &&
	test_cmp test_crayex_on7.exp test_crayex_on7.out &&
	count=`grep ^t test_crayex_on7.err | wc -l` &&
	test $count -eq 8 &&
	count=`grep "cannot perform on, dependency off" test_crayex_on7.err | wc -l` &&
	test $count -eq 8
'
test_expect_success 'powerman -q shows blade[4-7],cmm0,t[8-15] on' '
	$powerman -h $testaddr -q >test_crayex_query5.out &&
	makeoutput "blade[4-7],cmm0,t[8-15]" "blade[0-3],perif[0-7],t[0-7]" "" >test_crayex_query5.exp &&
	test_cmp test_crayex_query5.exp test_crayex_query5.out
'
# turn on blades and perif and then nodes
test_expect_success 'powerman -1 blade[0-3],perif[0-7] completes' '
	$powerman -h $testaddr -1 blade[0-3],perif[0-7] >test_crayex_on8.out &&
	echo Command completed successfully >test_crayex_on8.exp &&
	test_cmp test_crayex_on8.exp test_crayex_on8.out
'
test_expect_success 'powerman -q shows blade[0-7],cmm0,perif[0-7] on' '
	$powerman -h $testaddr -q >test_crayex_query6.out &&
	makeoutput "blade[0-7],cmm0,perif[0-7],t[8-15]" "t[0-7]" "" >test_crayex_query6.exp &&
	test_cmp test_crayex_query6.exp test_crayex_query6.out
'
test_expect_success 'powerman -1 t[0-15] completes' '
	$powerman -h $testaddr -1 t[0-15] >test_crayex_on6.out &&
	echo Command completed successfully >test_crayex_on6.exp &&
	test_cmp test_crayex_on6.exp test_crayex_on6.out
'
test_expect_success 'powerman -q shows blade[0-7],cmm0,t[0-15] on' '
	$powerman -h $testaddr -q >test_crayex_query7.out &&
	makeoutput "blade[0-7],cmm0,perif[0-7],t[0-15]" "" "" >test_crayex_query7.exp &&
	test_cmp test_crayex_query7.exp test_crayex_query7.out
'
# turn off leaves works
test_expect_success 'powerman -0 t[8-15] completes' '
	$powerman -h $testaddr -0 t[8-15] >test_crayex_off1.out &&
	echo Command completed successfully >test_crayex_off1.exp &&
	test_cmp test_crayex_off1.exp test_crayex_off1.out
'
test_expect_success 'powerman -q shows blade[0-7],cmm0,t[0-7] on' '
	$powerman -h $testaddr -q >test_crayex_query8.out &&
	makeoutput "blade[0-7],cmm0,perif[0-7],t[0-7]" "t[8-15]" "" >test_crayex_query8.exp &&
	test_cmp test_crayex_query8.exp test_crayex_query8.out
'
# turn off level 2 perif & blades, remaining leaves are now off
test_expect_success 'powerman -0 blade[0-7],perif[0-7] completes' '
	$powerman -h $testaddr -0 blade[0-7],perif[0-7] >test_crayex_off2.out &&
	echo Command completed successfully >test_crayex_off2.exp &&
	test_cmp test_crayex_off2.exp test_crayex_off2.out
'
test_expect_success 'powerman -q shows cmm0 on' '
	$powerman -h $testaddr -q >test_crayex_query9.out &&
	makeoutput "cmm0" "blade[0-7],perif[0-7],t[0-15]" "" >test_crayex_query9.exp &&
	test_cmp test_crayex_query9.exp test_crayex_query9.out
'
# turn off cmm works
test_expect_success 'powerman -0 cmm0 completes' '
	$powerman -h $testaddr -0 cmm0 >test_crayex_off3.out &&
	echo Command completed successfully >test_crayex_off3.exp &&
	test_cmp test_crayex_off3.exp test_crayex_off3.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayex_query10.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-7],t[0-15]" "" >test_crayex_query10.exp &&
	test_cmp test_crayex_query10.exp test_crayex_query10.out
'
# turn on everything doesn't work
# echo blade[0-7],cmm0,perif[0-7],t[0-15]:  >test_crayex_on9.exp &&
test_expect_success 'powerman -1 blade[0-7],cmm0,perif[0-7],t[0-15] doesnt work' '
	test_must_fail $powerman -h $testaddr -1 blade[0-7],cmm0,perif[0-7],t[0-15] >test_crayex_on9.out 2>test_crayex_on9.err &&
	echo Command completed with errors >>test_crayex_on9.exp &&
	test_cmp test_crayex_on9.exp test_crayex_on9.out &&
	echo cmm0: cannot turn on parent and child >>test_crayex_on9.err_exp
	count=`grep ^cmm0 test_crayex_on9.err | wc -l` &&
	test $count -eq 1 &&
	count=`grep ^blade test_crayex_on9.err | wc -l` &&
	test $count -eq 8 &&
	count=`grep ^t test_crayex_on9.err | wc -l` &&
	test $count -eq 16 &&
	count=`grep ^perif test_crayex_on9.err | wc -l` &&
	test $count -eq 8 &&
	count=`grep "cannot turn on parent and child" test_crayex_on9.err | wc -l` &&
	test $count -eq 33
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayex_query11.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-7],t[0-15]" "" >test_crayex_query11.exp &&
	test_cmp test_crayex_query11.exp test_crayex_query11.out
'
# turn everything on
test_expect_success 'powerman turn on everything' '
	$powerman -h $testaddr -1 cmm0 >test_crayex_on9A.out &&
	echo Command completed successfully >test_crayex_on9A.exp &&
	test_cmp test_crayex_on9A.exp test_crayex_on9A.out &&
	$powerman -h $testaddr -1 blade[0-7],perif[0-7] >test_crayex_on9B.out &&
	echo Command completed successfully >test_crayex_on9B.exp &&
	test_cmp test_crayex_on9B.exp test_crayex_on9B.out &&
	$powerman -h $testaddr -1 t[0-15] >test_crayex_on9C.out &&
	echo Command completed successfully >test_crayex_on9C.exp &&
	test_cmp test_crayex_on9C.exp test_crayex_on9C.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_crayex_query12.out &&
	makeoutput "blade[0-7],cmm0,perif[0-7],t[0-15]" "" "" >test_crayex_query12.exp &&
	test_cmp test_crayex_query12.exp test_crayex_query12.out
'
# turn off everything works
test_expect_success 'powerman -0 blade[0-3],cmm0,perif[0-7],t[0-15] completes' '
	$powerman -h $testaddr -0 blade[0-3],cmm0,perif[0-7],t[0-15] >test_crayex_off4.out &&
	echo Command completed successfully >test_crayex_off4.exp &&
	test_cmp test_crayex_off4.exp test_crayex_off4.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayex_query13.out &&
	makeoutput "" "blade[0-7],cmm0,perif[0-7],t[0-15]" "" >test_crayex_query13.exp &&
	test_cmp test_crayex_query13.exp test_crayex_query13.out
'
test_expect_success 'stop powerman daemon (crayex)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower hpe cray supercomputing ex chassis test - some unpopulated
#
# assume perif[4-7] and blade[4-7] do not exist
# as a result t[8-15] do not exist
#

test_expect_success 'create powerman.conf for chassis w/ 16 redfish nodes (crayexU)' '
	cat >powerman_cray_ex_U.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-cray-ex.dev"
	device "d0" "cray-ex" "$redfishdir/redfishpower -h cmm0,t[0-7],unused[0-7] --test-mode |&"
	node "cmm0,perif[0-3],blade[0-3],t[0-7]" "d0" "Enclosure,Perif[0-3],Blade[0-3],Node[0-7]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (crayexU)' '
	$powermand -Y -c powerman_cray_ex_U.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayexU_query1.out &&
	makeoutput "" "blade[0-3],cmm0,perif[0-3],t[0-7]" "" >test_crayexU_query1.exp &&
	test_cmp test_crayexU_query1.exp test_crayexU_query1.out
'
# turn on undefined stuff leads to errors
test_expect_success 'powerman fails on unpopulated targets' '
	test_must_fail $powerman -h $testaddr -1 blade4 &&
	test_must_fail $powerman -h $testaddr -1 perif5 &&
	test_must_fail $powerman -h $testaddr -1 t8
'
# turn on everything works
test_expect_success 'powerman turn on everything' '
	$powerman -h $testaddr -1 cmm0 >test_crayexU_on9A.out &&
	echo Command completed successfully >test_crayexU_on9A.exp &&
	test_cmp test_crayexU_on9A.exp test_crayexU_on9A.out &&
	$powerman -h $testaddr -1 blade[0-3],perif[0-3] >test_crayexU_on9B.out &&
	echo Command completed successfully >test_crayexU_on9B.exp &&
	test_cmp test_crayexU_on9B.exp test_crayexU_on9B.out &&
	$powerman -h $testaddr -1 t[0-7] >test_crayexU_on9C.out &&
	echo Command completed successfully >test_crayexU_on9C.exp &&
	test_cmp test_crayexU_on9C.exp test_crayexU_on9C.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_crayexU_query2.out &&
	makeoutput "blade[0-3],cmm0,perif[0-3],t[0-7]" "" "" >test_crayexU_query2.exp &&
	test_cmp test_crayexU_query2.exp test_crayexU_query2.out
'
# turn off everything works
test_expect_success 'powerman -0 blade[0-3],cmm0,perif[0-3],t[0-7] completes' '
	$powerman -h $testaddr -0 blade[0-3],cmm0,perif[0-3],t[0-7] >test_crayexU_off1.out &&
	echo Command completed successfully >test_crayexU_off1.exp &&
	test_cmp test_crayexU_off1.exp test_crayexU_off1.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_crayexU_query3.out &&
	makeoutput "" "blade[0-3],cmm0,perif[0-3],t[0-7]" "" >test_crayexU_query3.exp &&
	test_cmp test_crayexU_query3.exp test_crayexU_query3.out
'
test_expect_success 'stop powerman daemon (crayexU)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower hpe cray supercomputing ex chassis test - parent failure
# see issue #197
#

test_expect_success 'create powerman.conf for chassis w/ 16 redfish nodes (crayexfail)' '
	cat >powerman_cray_ex_fail.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-cray-ex.dev"
	device "d0" "cray-ex" "$redfishdir/redfishpower -h cmm0,t[0-15] --test-mode --test-fail-power-cmd-hosts=cmm0 |&"
	node "cmm0,perif[0-7],blade[0-7],t[0-15]" "d0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (crayexfail)' '
	$powermand -Y -c powerman_cray_ex_fail.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all unknown' '
	$powerman -h $testaddr -q >test_crayexfail_query1.out &&
	makeoutput "" "" "blade[0-7],cmm0,perif[0-7],t[0-15]" >test_crayexfail_query1.exp &&
	test_cmp test_crayexfail_query1.exp test_crayexfail_query1.out
'
test_expect_success 'powerman -1 t0 dependency error (parent error)' '
	test_must_fail $powerman -h $testaddr -1 t0 >test_crayexfail_on1.out 2>test_crayexfail_on1.err &&
	grep "cannot perform on, dependency error" test_crayexfail_on1.err
'
test_expect_success 'stop powerman daemon (crayexfail)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

test_done

# vi: set ft=sh
