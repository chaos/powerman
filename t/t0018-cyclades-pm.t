#!/bin/sh

test_description='Check Cyclades PM8, PM10, PM20, PM42 scripts'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
cyclades=$SHARNESS_BUILD_DIRECTORY/t/simulators/cyclades
pm8dev=$SHARNESS_TEST_SRCDIR/../etc/devices/cyclades-pm8.dev
pm10dev=$SHARNESS_TEST_SRCDIR/../etc/devices/cyclades-pm10.dev
pm20dev=$SHARNESS_TEST_SRCDIR/../etc/devices/cyclades-pm20.dev
pm42dev=$SHARNESS_TEST_SRCDIR/../etc/devices/cyclades-pm42.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11018

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create powerman.conf with pm8,pm10,pm20,pm42 devices (80 plugs)' '
	cat >powerman.conf <<-EOT
	listen "$testaddr"
	include "$pm8dev"
	include "$pm10dev"
	include "$pm20dev"
	include "$pm42dev"
	device "d0" "pm8" "$cyclades -p pm8 |&"
	device "d1" "pm10" "$cyclades -p pm10 |&"
	device "d2" "pm20" "$cyclades -p pm20 |&"
	device "d3" "pm42" "$cyclades -p pm42 |&"
	node "t[0-7]"  "d0"
	node "t[8-17]" "d1"
	node "t[18-37]" "d2"
	node "t[38-79]" "d3"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-79]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 t[0,8,18,38] works' '
	$powerman -h $testaddr -1 t[0,8,18,38] >on.out &&
	echo Command completed successfully >on.exp &&
	test_cmp on.exp on.out
'
test_expect_success 'powerman -q shows those nodes on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0,8,18,38]" "t[1-7,9-17,19-37,39-79]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -c t[0,8,18,38] works' '
	$powerman -h $testaddr -c t[0,8,18,38] >cycle.out &&
	echo Command completed successfully >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'powerman -q shows those nodes on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "t[0,8,18,38]" "t[1-7,9-17,19-37,39-79]" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 t[0,8,18,38] works' '
	$powerman -h $testaddr -0 t[0,8,18,38] >off.out &&
	echo Command completed successfully >off.exp &&
	test_cmp off.exp off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "t[0-79]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman -1 t[0-79] works' '
	$powerman -h $testaddr -1 t[0-79] >on2.out &&
	echo Command completed successfully >on2.exp &&
	test_cmp on2.exp on2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-79]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -c t[0-79] works' '
	$powerman -h $testaddr -c t[0-79] >cycle2.out &&
	echo Command completed successfully >cycle2.exp &&
	test_cmp cycle2.exp cycle2.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "t[0-79]" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -0 t[0-79] works' '
	$powerman -h $testaddr -0 t[0-79] >off2.out &&
	echo Command completed successfully >off2.exp &&
	test_cmp off2.exp off2.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "" "t[0-79]" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
