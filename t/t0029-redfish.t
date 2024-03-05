#!/bin/sh

test_description='Check redfishpower devices'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
redfishdir=$SHARNESS_BUILD_DIRECTORY/src/redfishpower
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11029


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

#
# redfishpower core tests
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-cray-r272z30.dev"
	device "d0" "redfishpower-cray-r272z30" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-15]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t[0,8] works' '
	$powerman -h $testaddr -1 t[0,8] >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows t[0,8] on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0,8]" "t[1-7,9-15]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t[0,8] works' '
	$powerman -h $testaddr -0 t[0,8] >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "" "t[0-15]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "t[0-15]" "" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -0 t[0-15] works' '
	$powerman -h $testaddr -0 t[0-15] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "" "t[0-15]" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -c t[0-15] works' '
	$powerman -h $testaddr -c t[0-15] >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "t[0-15]" "" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'powerman -c t[0-15] works again' '
	$powerman -h $testaddr -c t[0-15] >cycle2.out &&
	echo Command completed successfully >cycle2.exp &&
	test_cmp cycle2.exp cycle2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query6.out &&
	makeoutput "t[0-15]" "" "" >query6.exp &&
	test_cmp query6.exp query6.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower hpe cray supercomputing ex chassis test
#

test_expect_success 'create powerman.conf for chassis w/ 16 redfish nodes (crayex)' '
	cat >powerman_hpe_cray_supercomputing_ex_chassis.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-hpe-cray-supercomputing-ex-chassis.dev"
	device "d0" "redfishpower-CrayEX" "$redfishdir/redfishpower -h cmm0,t[0-15] --test-mode |&"
	node "cmm0,perif[0-7],blade[0-7],t[0-15]" "d0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (crayex)' '
	$powermand -Y -c powerman_hpe_cray_supercomputing_ex_chassis.conf &
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
	test_must_fail $powerman -h $testaddr --diag -1 blade[0-7] >test_crayex_on1.out &&
	echo blade[0-7]: ancestor off >test_crayex_on1.exp &&
	echo Command completed with errors >>test_crayex_on1.exp &&
	test_cmp test_crayex_on1.exp test_crayex_on1.out
'
test_expect_success 'powerman -1 perif[0-7] doesnt work' '
	test_must_fail $powerman -h $testaddr --diag -1 perif[0-7] >test_crayex_on2.out &&
	echo perif[0-7]: ancestor off >test_crayex_on2.exp &&
	echo Command completed with errors >>test_crayex_on2.exp &&
	test_cmp test_crayex_on2.exp test_crayex_on2.out
'
test_expect_success 'powerman -1 t[0-15] doesnt work' '
	test_must_fail $powerman -h $testaddr --diag -1 t[0-15] >test_crayex_on3.out &&
	echo t[0-15]: ancestor off >test_crayex_on3.exp &&
	echo Command completed with errors >>test_crayex_on3.exp &&
	test_cmp test_crayex_on3.exp test_crayex_on3.out
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
	test_must_fail $powerman -h $testaddr --diag -1 t[0-15] >test_crayex_on5.out &&
	echo t[0-15]: ancestor off >test_crayex_on5.exp &&
	echo Command completed with errors >>test_crayex_on5.exp &&
	test_cmp test_crayex_on5.exp test_crayex_on5.out
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
	test_must_fail $powerman -h $testaddr --diag -1 t[0-15] >test_crayex_on7.out &&
	echo t[0-7]: ancestor off >test_crayex_on7.exp &&
	echo Command completed with errors >>test_crayex_on7.exp &&
	test_cmp test_crayex_on7.exp test_crayex_on7.out
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
test_expect_success 'powerman -1 blade[0-7],cmm0,perif[0-7],t[0-15] doesnt work' '
	test_must_fail $powerman -h $testaddr --diag -1 blade[0-7],cmm0,perif[0-7],t[0-15] >test_crayex_on9.out &&
	echo blade[0-7],cmm0,perif[0-7],t[0-15]: cannot turn on parent and child >test_crayex_on9.exp &&
	echo Command completed with errors >>test_crayex_on9.exp &&
	test_cmp test_crayex_on9.exp test_crayex_on9.out
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
	cat >powerman_hpe_cray_supercomputing_ex_chassis_U.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-hpe-cray-supercomputing-ex-chassis.dev"
	device "d0" "redfishpower-CrayEX" "$redfishdir/redfishpower -h cmm0,t[0-7],unused[0-7] --test-mode |&"
	node "cmm0,perif[0-3],blade[0-3],t[0-7]" "d0" "Enclosure,Perif[0-3],Blade[0-3],Node[0-7]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (crayexU)' '
	$powermand -Y -c powerman_hpe_cray_supercomputing_ex_chassis_U.conf &
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
# redfishpower hpe cray supercomputing ex chassis test - rabbit
#
# rabbit is descendant of perif 7
#

test_expect_success 'create powerman.conf for chassis w/ 16 redfish nodes (crayexR)' '
	cat >powerman_hpe_cray_supercomputing_ex_chassis_U.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-hpe-cray-supercomputing-ex-chassis.dev"
	device "d0" "redfishpower-CrayEX-rabbit-s4s7" "$redfishdir/redfishpower -h cmm0,t[0-15],rabbit --test-mode |&"
	node "cmm0,perif[0-4,7],blade[0-7],t[0-15],rabbit" "d0" "Enclosure,Perif[0-4,7],Blade[0-7],Node[0-16]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (crayexR)' '
	$powermand -Y -c powerman_hpe_cray_supercomputing_ex_chassis_U.conf &
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
	test_must_fail $powerman -h $testaddr --diag -1 rabbit >test_crayexR_on1.out &&
	echo rabbit: ancestor off >test_crayexR_on1.exp &&
	echo Command completed with errors >>test_crayexR_on1.exp &&
	test_cmp test_crayexR_on1.exp test_crayexR_on1.out
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
	test_must_fail $powerman -h $testaddr --diag -1 rabbit >test_crayexR_on3.out &&
	echo rabbit: ancestor off >test_crayexR_on3.exp &&
	echo Command completed with errors >>test_crayexR_on3.exp &&
	test_cmp test_crayexR_on3.exp test_crayexR_on3.out
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
