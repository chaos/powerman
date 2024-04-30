#!/bin/sh

test_description='Cover redfishpower specific configurations'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
redfishdir=$SHARNESS_BUILD_DIRECTORY/src/redfishpower
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices
testdevicesdir=$SHARNESS_TEST_SRCDIR/etc

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11034


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

#
# redfishpower fail hosts coverage
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (failhosts)' '
	cat >powerman_fail_hosts.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/redfishpower-cray-r272z30.dev"
	device "d0" "redfishpower-cray-r272z30" "$redfishdir/redfishpower -h t[0-15] --test-mode --test-fail-power-cmd-hosts=t[8-15] |&"
	node "t[0-15]" "d0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (failhosts)' '
	$powermand -Y -c powerman_fail_hosts.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows t[0-7] off, t[8-15] unknown' '
	$powerman -h $testaddr -q >test_failhosts_query.out &&
	makeoutput "" "t[0-7]" "t[8-15]" >test_failhosts_query.exp &&
	test_cmp test_failhosts_query.exp test_failhosts_query.out
'
test_expect_success 'powerman -1 t[0-15] completes' '
	$powerman -h $testaddr -1 t[0-15] >test_failhosts_on.out &&
	echo Command completed successfully >test_failhosts_on.exp &&
	test_cmp test_failhosts_on.exp test_failhosts_on.out
'
test_expect_success 'powerman -q shows t[0-7] on' '
	$powerman -h $testaddr -q >test_failhosts_query2.out &&
	makeoutput "t[0-7]" "" "t[8-15]" >test_failhosts_query2.exp &&
	test_cmp test_failhosts_query2.exp test_failhosts_query2.out
'
test_expect_success 'stop powerman daemon (failhosts)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower setplugs coverage
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (setplugs)' '
	cat >powerman_setplugs.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setplugs.dev"
	device "d0" "redfishpower-setplugs" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setplugs)' '
	$powermand -Y -c powerman_setplugs.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_setplugs_query.out &&
	makeoutput "" "t[0-15]" "" >test_setplugs_query.exp &&
	test_cmp test_setplugs_query.exp test_setplugs_query.out
'
test_expect_success 'powerman -T -q shows plugs are being used' '
	$powerman -h $testaddr -T -q > test_setplugs_query_T.out &&
	grep "Node15: off" test_setplugs_query_T.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >test_setplugs_on.out &&
	echo Command completed successfully >test_setplugs_on.exp &&
	test_cmp test_setplugs_on.exp test_setplugs_on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_setplugs_query2.out &&
	makeoutput "t[0-15]" "" "" >test_setplugs_query2.exp &&
	test_cmp test_setplugs_query2.exp test_setplugs_query2.out
'
test_expect_success 'stop powerman daemon (setplugs)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower setpath coverage
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (setpath)' '
	cat >powerman_setpath.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setpath.dev"
	device "d0" "redfishpower-setpath" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setpath' '
	$powermand -Y -c powerman_setpath.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_setpath_query.out &&
	makeoutput "" "t[0-15]" "" >test_setpath_query.exp &&
	test_cmp test_setpath_query.exp test_setpath_query.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >test_setpath_on.out &&
	echo Command completed successfully >test_setpath_on.exp &&
	test_cmp test_setpath_on.exp test_setpath_on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_setpath_query2.out &&
	makeoutput "t[0-15]" "" "" >test_setpath_query2.exp &&
	test_cmp test_setpath_query2.exp test_setpath_query2.out
'
test_expect_success 'stop powerman daemon (setpath)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

# Note, verbose output can mess up the device script's interpretation of power status,
# so restart powermand with verbose mode to run telemetry specific tests.
test_expect_success 'create powerman.conf for 16 cray redfish nodes (setpath2)' '
	cat >powerman_setpath2.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-setpath.dev"
	device "d0" "redfishpower-setpath" "$redfishdir/redfishpower -h t[0-15] --test-mode -vv |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (setpath2)' '
	$powermand -Y -c powerman_setpath2.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
# Note: redfishpower-setpath.dev sets different paths for different
# nodes.  Following tests check that correct path has been used.
test_expect_success 'powerman -T -1 shows correct path being used (t0)' '
	$powerman -h $testaddr -T -1 t0 > test_setpath2_on_T1.out &&
	grep "plugname=Node0" test_setpath2_on_T1.out | grep "path=" | grep Group0
'
test_expect_success 'powerman -T -1 shows correct path being used (t5)' '
	$powerman -h $testaddr -T -1 t5 > test_setpath2_on_T2.out &&
	grep "plugname=Node5" test_setpath2_on_T2.out | grep "path=" | grep Group1
'
test_expect_success 'powerman -T -1 shows correct path being used (t10)' '
	$powerman -h $testaddr -T -1 t10 > test_setpath2_on_T3.out &&
	grep "plugname=Node10" test_setpath2_on_T3.out | grep "path=" | grep Default
'
test_expect_success 'stop powerman daemon (setpath2)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower plug substitution coverage
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (plugsub)' '
	cat >powerman_plugsub.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-plugsub.dev"
	device "d0" "redfishpower-plugsub" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (plugsub)' '
	$powermand -Y -c powerman_plugsub.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_plugsub_query.out &&
	makeoutput "" "t[0-15]" "" >test_plugsub_query.exp &&
	test_cmp test_plugsub_query.exp test_plugsub_query.out
'
test_expect_success 'powerman -1 t[0-15] works' '
	$powerman -h $testaddr -1 t[0-15] >test_plugsub_on.out &&
	echo Command completed successfully >test_plugsub_on.exp &&
	test_cmp test_plugsub_on.exp test_plugsub_on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_plugsub_query2.out &&
	makeoutput "t[0-15]" "" "" >test_plugsub_query2.exp &&
	test_cmp test_plugsub_query2.exp test_plugsub_query2.out
'
test_expect_success 'stop powerman daemon (plugsub)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

# Note, verbose output can mess up the device script's interpretation of power status,
# so restart powermand with verbose mode to run telemetry specific tests.
test_expect_success 'create powerman.conf for 16 cray redfish nodes (plugsub2)' '
	cat >powerman_plugsub2.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-plugsub.dev"
	device "d0" "redfishpower-plugsub" "$redfishdir/redfishpower -h t[0-15] --test-mode -vv |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (plugsub2)' '
	$powermand -Y -c powerman_plugsub2.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
# Note: redfishpower-plugsub.dev sets different paths for different
# nodes.  Following tests check that correct path has been used.
test_expect_success 'powerman -T -1 shows correct path being used (t0)' '
	$powerman -h $testaddr -T -1 t0 > test_plugsub2_on_T1.out &&
	grep "plugname=Node0" test_plugsub2_on_T1.out | grep "path=" | grep Group0-Node0
'
test_expect_success 'powerman -T -1 shows correct path being used (t8)' '
	$powerman -h $testaddr -T -1 t8 > test_plugsub2_on_T2.out &&
	grep "plugname=Node8" test_plugsub2_on_T2.out | grep "path=" | grep Default-Node8
'
test_expect_success 'stop powerman daemon (plugsub2)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower plug substitution coverage w/ hypothetical blades
#
# Blades[0-3] assumed to go through t0
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (plugsubB)' '
	cat >powerman_plugsubB.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-plugsub-blades.dev"
	device "d0" "redfishpower-plugsub-blades" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0" "Node[0-15]"
	node "blade[0-3]" "d0" "Blade[0-3]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (plugsubB)' '
	$powermand -Y -c powerman_plugsubB.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_plugsubB_query.out &&
	makeoutput "" "blade[0-3],t[0-15]" "" >test_plugsubB_query.exp &&
	test_cmp test_plugsubB_query.exp test_plugsubB_query.out
'
test_expect_success 'powerman -1 blade[0-3],t[0-15] works' '
	$powerman -h $testaddr -1 blade[0-3],t[0-15] >test_plugsubB_on.out &&
	echo Command completed successfully >test_plugsubB_on.exp &&
	test_cmp test_plugsubB_on.exp test_plugsubB_on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_plugsubB_query2.out &&
	makeoutput "blade[0-3],t[0-15]" "" "" >test_plugsubB_query2.exp &&
	test_cmp test_plugsubB_query2.exp test_plugsubB_query2.out
'
test_expect_success 'stop powerman daemon (plugsubB)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

# Note, verbose output can mess up the device script's interpretation of power status,
# so restart powermand with verbose mode to run telemetry specific tests.
test_expect_success 'create powerman.conf for 16 cray redfish nodes (plugsubB2)' '
	cat >powerman_plugsubB2.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-plugsub-blades.dev"
	device "d0" "redfishpower-plugsub-blades" "$redfishdir/redfishpower -h t[0-15] --test-mode -vv |&"
	node "t[0-15]" "d0" "Node[0-15]"
	node "blade[0-3]" "d0" "Blade[0-3]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (plugsubB2)' '
	$powermand -Y -c powerman_plugsubB2.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -T -1 shows correct path being used (t0)' '
	$powerman -h $testaddr -T -1 t0 > test_plugsubB2_on_T1.out &&
	grep "plugname=Node0" test_plugsubB2_on_T1.out | grep "path=" | grep Default-Node0
'
test_expect_success 'powerman -T -1 shows correct path being used (t8)' '
	$powerman -h $testaddr -T -1 blade0 > test_plugsubB2_on_T2.out &&
	grep "plugname=Blade0" test_plugsubB2_on_T2.out | grep "path=" | grep Default-Blade0
'
test_expect_success 'stop powerman daemon (plugsubB2)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower parent coverage (2 levels)
#
# - t0 is the parent of t[1-15]
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (parents2L)' '
	cat >powerman_parents2L.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-parents-2-levels.dev"
	device "d0" "redfishpower-parents-2-levels" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (parents2L)' '
	$powermand -Y -c powerman_parents2L.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_parents2L_query1.out &&
	makeoutput "" "t[0-15]" "" >test_parents2L_query1.exp &&
	test_cmp test_parents2L_query1.exp test_parents2L_query1.out
'
# test children can't be turned on if parent off
test_expect_success 'powerman -1 t[1-15] completes' '
	$powerman -h $testaddr -1 t[1-15] >test_parents2L_on1.out &&
	echo Command completed successfully >test_parents2L_on1.exp &&
	test_cmp test_parents2L_on1.exp test_parents2L_on1.out &&
	$powerman -h $testaddr -T -1 t[1-15] >test_parents2L_on1T.out &&
	grep "cannot perform on, dependency off" test_parents2L_on1T.out
'
test_expect_success 'powerman -q shows all off still' '
	$powerman -h $testaddr -q >test_parents2L_query2.out &&
	makeoutput "" "t[0-15]" "" >test_parents2L_query2.exp &&
	test_cmp test_parents2L_query2.exp test_parents2L_query2.out
'
# test children can be turned on if parent on
test_expect_success 'powerman -1 t0 completes' '
	$powerman -h $testaddr -1 t0 >test_parents2L_on2.out &&
	echo Command completed successfully >test_parents2L_on2.exp &&
	test_cmp test_parents2L_on2.exp test_parents2L_on2.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >test_parents2L_query3.out &&
	makeoutput "t0" "t[1-15]" "" >test_parents2L_query3.exp &&
	test_cmp test_parents2L_query3.exp test_parents2L_query3.out
'
test_expect_success 'powerman -1 t[1-15] completes' '
	$powerman -h $testaddr -1 t[1-15] >test_parents2L_on3.out &&
	echo Command completed successfully >test_parents2L_on3.exp &&
	test_cmp test_parents2L_on3.exp test_parents2L_on3.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >test_parents2L_query4.out &&
	makeoutput "t[0-15]" "" "" >test_parents2L_query4.exp &&
	test_cmp test_parents2L_query4.exp test_parents2L_query4.out
'
# turn everything off by group
test_expect_success 'powerman -0 t[1-15] completes' '
	$powerman -h $testaddr -0 t[1-15] >test_parents2L_off1.out &&
	echo Command completed successfully >test_parents2L_off1.exp &&
	test_cmp test_parents2L_off1.exp test_parents2L_off1.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >test_parents2L_query5.out &&
	makeoutput "t0" "t[1-15]" "" >test_parents2L_query5.exp &&
	test_cmp test_parents2L_query5.exp test_parents2L_query5.out
'
test_expect_success 'powerman -0 t0 completes' '
	$powerman -h $testaddr -0 t0 >test_parents2L_off2.out &&
	echo Command completed successfully >test_parents2L_off2.exp &&
	test_cmp test_parents2L_off2.exp test_parents2L_off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_parents2L_query6.out &&
	makeoutput "" "t[0-15]" "" >test_parents2L_query6.exp &&
	test_cmp test_parents2L_query6.exp test_parents2L_query6.out
'
# turn off nodes with parent off succeeds
test_expect_success 'powerman -0 t[1-15] completes' '
	$powerman -h $testaddr -0 t[1-15] >test_parents2L_off3.out &&
	echo Command completed successfully >test_parents2L_off3.exp &&
	test_cmp test_parents2L_off3.exp test_parents2L_off3.out &&
	$powerman -h $testaddr -T -0 t[1-15] >test_parents2L_off3T.out &&
	grep "Node1" test_parents2L_off3T.out | grep "ok"
'
# turn on parents & children at same time fails
test_expect_success 'powerman -1 t[0-15] completes but doesnt work' '
	$powerman -h $testaddr -1 t[0-15] >test_parents2L_on4.out &&
	echo Command completed successfully >test_parents2L_on4.exp &&
	test_cmp test_parents2L_on4.exp test_parents2L_on4.out &&
	$powerman -h $testaddr -T -1 t[0-15] >test_parents2L_on4T.out &&
	grep "cannot turn on parent" test_parents2L_on4T.out
'
test_expect_success 'powerman -1 t8,t0 completes but doesnt work' '
	$powerman -h $testaddr -1 t[8,0] >test_parents2L_on5.out &&
	echo Command completed successfully >test_parents2L_on5.exp &&
	test_cmp test_parents2L_on5.exp test_parents2L_on5.out &&
	$powerman -h $testaddr -T -1 t[8,0] >test_parents2L_on5T.out &&
	grep "cannot turn on parent" test_parents2L_on5T.out
'
# turn on everything
test_expect_success 'powerman -0 t0 and then t[1-15] completes' '
	$powerman -h $testaddr -1 t0 >test_parents2L_on6.out &&
	echo Command completed successfully >test_parents2L_on6.exp &&
	test_cmp test_parents2L_on6.exp test_parents2L_on6.out &&
	$powerman -h $testaddr -1 t[1-15] >test_parents2L_on6B.out &&
	echo Command completed successfully >test_parents2L_on6B.exp &&
	test_cmp test_parents2L_on6B.exp test_parents2L_on6B.out
'
test_expect_success 'powerman -q shows t[0-15] on' '
	$powerman -h $testaddr -q >test_parents2L_query7.out &&
	makeoutput "t[0-15]" "" "" >test_parents2L_query7.exp &&
	test_cmp test_parents2L_query7.exp test_parents2L_query7.out
'
# turn off everything at same time, dependencies handled
test_expect_success 'powerman -0 t[0-15] completes' '
	$powerman -h $testaddr -0 t0 >test_parents2L_off3.out &&
	echo Command completed successfully >test_parents2L_off3.exp &&
	test_cmp test_parents2L_off3.exp test_parents2L_off3.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_parents2L_query8.out &&
	makeoutput "" "t[0-15]" "" >test_parents2L_query8.exp &&
	test_cmp test_parents2L_query8.exp test_parents2L_query8.out
'
test_expect_success 'stop powerman daemon (parents2L)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower parent coverage (3 levels)
#
# - t0 is the parent of t[1-3]
# - t1 is the parent of t[4-7]
# - t2 is the parent of t[8-11]
# - t3 is the parent of t[12-15]
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (parents3L)' '
	cat >powerman_parents3L.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-parents-3-levels.dev"
	device "d0" "redfishpower-parents-3-levels" "$redfishdir/redfishpower -h t[0-15] --test-mode |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (parents3L)' '
	$powermand -Y -c powerman_parents3L.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_parents3L_query1.out &&
	makeoutput "" "t[0-15]" "" >test_parents3L_query1.exp &&
	test_cmp test_parents3L_query1.exp test_parents3L_query1.out
'
# test children can't be turned on if parent off
test_expect_success 'powerman -1 t[1-3] completes but doesnt work' '
	$powerman -h $testaddr -1 t[1-3] >test_parents3L_on1.out &&
	echo Command completed successfully >test_parents3L_on1.exp &&
	test_cmp test_parents3L_on1.exp test_parents3L_on1.out &&
	$powerman -h $testaddr -T -1 t[1-3] >test_parents3L_on1T.out &&
	grep "cannot perform on, dependency off" test_parents3L_on1T.out
'
test_expect_success 'powerman -1 t[4-15] completes but doesnt work' '
	$powerman -h $testaddr -1 t[4-15] >test_parents3L_on2.out &&
	echo Command completed successfully >test_parents3L_on2.exp &&
	test_cmp test_parents3L_on2.exp test_parents3L_on2.out &&
	$powerman -h $testaddr -T -1 t[4-15] >test_parents3L_on2T.out &&
	grep "cannot perform on, dependency off" test_parents3L_on2T.out
'
test_expect_success 'powerman -q shows all off still' '
	$powerman -h $testaddr -q >test_parents3L_query2.out &&
	makeoutput "" "t[0-15]" "" >test_parents3L_query2.exp &&
	test_cmp test_parents3L_query2.exp test_parents3L_query2.out
'
# test level 2 children can be turned on if parent on
test_expect_success 'powerman -1 t0 completes' '
	$powerman -h $testaddr -1 t0 >test_parents3L_on3.out &&
	echo Command completed successfully >test_parents3L_on3.exp &&
	test_cmp test_parents3L_on3.exp test_parents3L_on3.out
'
test_expect_success 'powerman -q shows t0 on' '
	$powerman -h $testaddr -q >test_parents3L_query3.out &&
	makeoutput "t0" "t[1-15]" "" >test_parents3L_query3.exp &&
	test_cmp test_parents3L_query3.exp test_parents3L_query3.out
'
test_expect_success 'powerman -1 t1 completes' '
	$powerman -h $testaddr -1 t1 >test_parents3L_on4.out &&
	echo Command completed successfully >test_parents3L_on4.exp &&
	test_cmp test_parents3L_on4.exp test_parents3L_on4.out
'
test_expect_success 'powerman -q shows t[0-1] on' '
	$powerman -h $testaddr -q >test_parents3L_query4.out &&
	makeoutput "t[0-1]" "t[2-15]" "" >test_parents3L_query4.exp &&
	test_cmp test_parents3L_query4.exp test_parents3L_query4.out
'
# test level 3 children can be turned on if parents on
test_expect_success 'powerman -1 t[4-15] completes' '
	$powerman -h $testaddr -1 t[4-15] >test_parents3L_on5.out &&
	echo Command completed successfully >test_parents3L_on5.exp &&
	test_cmp test_parents3L_on5.exp test_parents3L_on5.out
'
test_expect_success 'powerman -q shows t[0-1,4-7] on' '
	$powerman -h $testaddr -q >test_parents3L_query5.out &&
	makeoutput "t[0-1,4-7]" "t[2-3,8-15]" "" >test_parents3L_query5.exp &&
	test_cmp test_parents3L_query5.exp test_parents3L_query5.out
'
# test level 2 & 3 children cannot be turned on together
test_expect_success 'powerman -1 t[2-3,8-15] completes but doesnt work' '
	$powerman -h $testaddr -1 t[2-3,8-15] >test_parents3L_on6.out &&
	echo Command completed successfully >test_parents3L_on6.exp &&
	test_cmp test_parents3L_on6.exp test_parents3L_on6.out &&
	$powerman -h $testaddr -T -1 t[2-3,8-15] >test_parents3L_on6T.out &&
	grep "cannot turn on parent and child" test_parents3L_on6T.out
'
test_expect_success 'powerman -1 t[8-10,2,11-15,3] completes but doesnt work' '
	$powerman -h $testaddr -1 t[8-10,2,11-15,3] >test_parents3L_on7.out &&
	echo Command completed successfully >test_parents3L_on7.exp &&
	test_cmp test_parents3L_on7.exp test_parents3L_on7.out &&
	$powerman -h $testaddr -T -1 t[8-10,2,11-15,3] >test_parents3L_on7T.out &&
	grep "cannot turn on parent and child" test_parents3L_on7T.out
'
test_expect_success 'powerman -q shows t[0-1,4-7] on' '
	$powerman -h $testaddr -q >test_parents3L_query6.out &&
	makeoutput "t[0-1,4-7]" "t[2-3,8-15]" "" >test_parents3L_query6.exp &&
	test_cmp test_parents3L_query6.exp test_parents3L_query6.out
'
# turn on t2
test_expect_success 'powerman -1 t2 completes' '
	$powerman -h $testaddr -1 t2 >test_parents3L_on8.out &&
	echo Command completed successfully >test_parents3L_on8.exp &&
	test_cmp test_parents3L_on8.exp test_parents3L_on8.out
'
test_expect_success 'powerman -q shows t[0-2,4-7] on' '
	$powerman -h $testaddr -q >test_parents3L_query7.out &&
	makeoutput "t[0-2,4-7]" "t[3,8-15]" "" >test_parents3L_query7.exp &&
	test_cmp test_parents3L_query7.exp test_parents3L_query7.out
'
# turn on level 2 & 3 nodes that are not dependent on each other works
test_expect_success 'powerman -1 t[3,8-11] completes' '
	$powerman -h $testaddr -1 t[3,8-11] >test_parents3L_on9.out &&
	echo Command completed successfully >test_parents3L_on9.exp &&
	test_cmp test_parents3L_on9.exp test_parents3L_on9.out
'
test_expect_success 'powerman -q shows t[0-11] on' '
	$powerman -h $testaddr -q >test_parents3L_query8.out &&
	makeoutput "t[0-11]" "t[12-15]" "" >test_parents3L_query8.exp &&
	test_cmp test_parents3L_query8.exp test_parents3L_query8.out
'
# turn everything off level 2 nodes, some have children some don't
test_expect_success 'powerman -0 t[2-3] completes' '
	$powerman -h $testaddr -0 t[2-3] >test_parents3L_off1.out &&
	echo Command completed successfully >test_parents3L_off1.exp &&
	test_cmp test_parents3L_off1.exp test_parents3L_off1.out
'
test_expect_success 'powerman -q shows t[0-1,4-7] on' '
	$powerman -h $testaddr -q >test_parents3L_query9.out &&
	makeoutput "t[0-1,4-7]" "t[2-3,8-15]" "" >test_parents3L_query9.exp &&
	test_cmp test_parents3L_query9.exp test_parents3L_query9.out
'
# turn everything else off
test_expect_success 'powerman -0 t0 completes' '
	$powerman -h $testaddr -0 t0 >test_parents3L_off2.out &&
	echo Command completed successfully >test_parents3L_off2.exp &&
	test_cmp test_parents3L_off2.exp test_parents3L_off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_parents3L_query10.out &&
	makeoutput "" "t[0-15]" "" >test_parents3L_query10.exp &&
	test_cmp test_parents3L_query10.exp test_parents3L_query10.out
'
# turn everything on
test_expect_success 'powerman turn on everything' '
	$powerman -h $testaddr -1 t0 >test_parents3L_on10A.out &&
	echo Command completed successfully >test_parents3L_on10A.exp &&
	test_cmp test_parents3L_on10A.exp test_parents3L_on10A.out &&
	$powerman -h $testaddr -1 t[1-3] >test_parents3L_on10B.out &&
	echo Command completed successfully >test_parents3L_on10B.exp &&
	test_cmp test_parents3L_on10B.exp test_parents3L_on10B.out &&
	$powerman -h $testaddr -1 t[4-15] >test_parents3L_on10C.out &&
	echo Command completed successfully >test_parents3L_on10C.exp &&
	test_cmp test_parents3L_on10C.exp test_parents3L_on10C.out
'
test_expect_success 'powerman -q shows t[0-15] on' '
	$powerman -h $testaddr -q >test_parents3L_query11.out &&
	makeoutput "t[0-15]" "" "" >test_parents3L_query11.exp &&
	test_cmp test_parents3L_query11.exp test_parents3L_query11.out
'
# turn off everything at same time, dependencies handled
test_expect_success 'powerman -0 t[0-15] completes' '
	$powerman -h $testaddr -0 t0 >test_parents3L_off3.out &&
	echo Command completed successfully >test_parents3L_off3.exp &&
	test_cmp test_parents3L_off3.exp test_parents3L_off3.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_parents3L_query11.out &&
	makeoutput "" "t[0-15]" "" >test_parents3L_query11.exp &&
	test_cmp test_parents3L_query11.exp test_parents3L_query11.out
'
test_expect_success 'stop powerman daemon (parents3L)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# redfishpower parent coverage (3 levels) w/ node that always fails
#
# - t0 is the parent of t[1-3]
# - t1 is the parent of t[4-7]
# - t2 is the parent of t[8-11]
# - t3 is the parent of t[12-15]
#

test_expect_success 'create powerman.conf for 16 cray redfish nodes (parents3Lbad)' '
	cat >powerman_parents3Lbad.conf <<-EOT
	listen "$testaddr"
	include "$testdevicesdir/redfishpower-parents-3-levels.dev"
	device "d0" "redfishpower-parents-3-levels" "$redfishdir/redfishpower -h t[0-15] --test-mode --test-fail-power-cmd-hosts=t3 |&"
	node "t[0-15]" "d0" "Node[0-15]"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start (parents3Lbad)' '
	$powermand -Y -c powerman_parents3Lbad.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
# Note, t0 is off, so everything below it is defined as off
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >test_parents3Lbad_query1.out &&
	makeoutput "" "t[0-15]" "" >test_parents3Lbad_query1.exp &&
	test_cmp test_parents3Lbad_query1.exp test_parents3Lbad_query1.out
'
# try turning everything on
test_expect_success 'powerman turn on everything' '
	$powerman -h $testaddr -1 t0 >test_parents3Lbad_on1A.out &&
	echo Command completed successfully >test_parents3Lbad_on1A.exp &&
	test_cmp test_parents3Lbad_on1A.exp test_parents3Lbad_on1A.out &&
	$powerman -h $testaddr -1 t[1-3] >test_parents3Lbad_on1B.out &&
	echo Command completed successfully >test_parents3Lbad_on1B.exp &&
	test_cmp test_parents3Lbad_on1B.exp test_parents3Lbad_on1B.out &&
	$powerman -h $testaddr -1 t[4-15] >test_parents3Lbad_on1C.out &&
	echo Command completed successfully >test_parents3Lbad_on1C.exp &&
	test_cmp test_parents3Lbad_on1C.exp test_parents3Lbad_on1C.out
'
test_expect_success 'powerman -q shows t[0-2,4-11] on, t3 and children unknown' '
	$powerman -h $testaddr -q >test_parents3Lbad_query2.out &&
	makeoutput "t[0-2,4-11]" "" "t[3,12-15]" >test_parents3Lbad_query2.exp &&
	test_cmp test_parents3Lbad_query2.exp test_parents3Lbad_query2.out
'
# try turning on some children of the bad node
test_expect_success 'powerman -1 t[12-15] completes' '
	$powerman -h $testaddr -1 t[12-15] >test_parents3Lbad_on2.out &&
	echo Command completed successfully >test_parents3Lbad_on2.exp &&
	test_cmp test_parents3Lbad_on2.exp test_parents3Lbad_on2.out &&
	$powerman -h $testaddr -T -1 t[12-15] >test_parents3Lbad_on2T.out &&
	grep "Node12" test_parents3Lbad_on2T.out | grep "cannot perform on, dependency error"
'
test_expect_success 'powerman -q shows t[0-2,4-11] on, t3 and children unknown' '
	$powerman -h $testaddr -q >test_parents3Lbad_query3.out &&
	makeoutput "t[0-2,4-11]" "" "t[3,12-15]" >test_parents3Lbad_query3.exp &&
	test_cmp test_parents3Lbad_query3.exp test_parents3Lbad_query3.out
'
test_expect_success 'stop powerman daemon (parents3Lbad)' '
	kill -15 $(cat powermand.pid) &&
	wait
'

#
# options
#
# libcurl specific and not testable under test-mode, so we just check the options work
#

test_expect_success 'header option setting appears to work' '
	echo "quit" | $redfishdir/redfishpower -h t[0-15] --test-mode --header="my content header" 2> header.err
	grep "header = my content header" header.err
'
test_expect_success 'auth option setting appears to work' '
	echo "quit" | $redfishdir/redfishpower -h t[0-15] --test-mode --auth="foo:bar" 2> auth.err
	grep "auth = foo:bar" auth.err
'

#
# valgrind
#

if valgrind --version; then
	test_set_prereq HAVE_VALGRIND
fi

# test input largely follows Cray EX chassis
test_expect_success HAVE_VALGRIND 'create redfishpower test input' '
	cat >redfishpower.in <<-EOT
	auth USER:PASS
	setheader Content-Type:application/json
	setplugs Enclosure 0
	setplugs Perif[0-7],Blade[0-7] 0 Enclosure
	setplugs Node[0-1] [1-2] Blade0
	setplugs Node[2-3] [3-4] Blade1
	setplugs Node[4-5] [5-6] Blade2
	setplugs Node[6-7] [7-8] Blade3
	setplugs Node[8-9] [9-10] Blade4
	setplugs Node[10-11] [11-12] Blade5
	setplugs Node[12-13] [13-14] Blade6
	setplugs Node[14-15] [15-16] Blade7
	setpath Enclosure,Perif[0-7],Blade[0-7] stat redfish/v1/Chassis/{{plug}}
	setpath Enclosure,Perif[0-7],Blade[0-7] on redfish/v1/Chassis/{{plug}}/Actions/Chassis.Reset {\"ResetType\":\"On\"}
	setpath Enclosure,Perif[0-7],Blade[0-7] off redfish/v1/Chassis/{{plug}}/Actions/Chassis.Reset {\"ResetType\":\"ForceOff\"}
	setpath Node[0-15] stat redfish/v1/Systems/Node0
	setpath Node[0-15] on redfish/v1/Systems/Node0/Actions/ComputerSystem.Reset {\"ResetType\":\"On\"}
	setpath Node[0-15] off redfish/v1/Systems/Node0/Actions/ComputerSystem.Reset {\"ResetType\":\"ForceOff\"}
	settimeout 60
	stat
	stat Enclosure,Perif[0-7],Blade[0-7],Node[0-15]
	on Enclosure,Perif[0-7],Blade[0-7],Node[0-15]
	on Enclosure
	on Perif[0-7],Blade[0-7]
	on Node[0-15]
	stat
	stat Enclosure,Perif[0-7],Blade[0-7],Node[0-15]
	off Node[0-15]
	off Perif[0-7],Blade[0-7]
	off Enclosure
	EOT
'

test_expect_success HAVE_VALGRIND 'run redfishpower under valgrind' '
	cat redfishpower.in | valgrind --tool=memcheck --leak-check=full \
	    --error-exitcode=1 --gen-suppressions=all \
	    $redfishdir/redfishpower -h cmm0,t[0-15] --test-mode
'

test_done

# vi: set ft=sh
