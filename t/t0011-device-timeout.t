#!/bin/sh

test_description='Test Device timeouts'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11011

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

test_expect_success 'create test powerman.conf with broken status_all' '
	cat >powerman.conf <<-EOT
	specification "vpcbroke" {
	    timeout 	2.0
	    plug name { "0" "1" "2" "3" "4" "5" "6" "7" "8"
	                "9" "10" "11" "12" "13" "14" "15" }
	    script login {
	        send "login\n"
	        expect "[0-9]* OK\n"
	        expect "[0-9]* vpc> "
	    }
	    script logout {
	        send "logoff\n"
	        expect "[0-9]* OK\n"
	    }
	    # Hacked not to work
	    script status_all {
	        send "stat *\n"
	        expect "WONTGETTHIS"
	    }
	}
	listen "$testaddr"
	device "test0" "vpcbroke" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q fails due to timeout' '
	test_must_fail $powerman -h $testaddr -q >query.out
'
test_expect_success 'and command had expected output' '
	echo test0: action timed out waiting for expected response >query.exp &&
	makeoutput "" "" "t[0-15]" >>query.exp &&
	echo Query completed with errors >>query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'stop powerman daemon' '
        kill -15 $(cat powermand.pid) &&
        wait
'
test_expect_success 'add another device (16 more plugs) but this one works' '
	cat >>powerman.conf <<-EOT
	include "$vpcdev"
	device "test1" "vpc" "$vpcd |&"
	node "t[16-31]" "test1"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q t[14-18] times out' '
	test_must_fail $powerman -h $testaddr -q t[14-18] >query2.out
'
test_expect_success 'and command partially succeeded' '
	echo test0: action timed out waiting for expected response \
		>query2.exp &&
	makeoutput "" "t[16-18]" "t[14-15]" >>query2.exp &&
	echo Query completed with errors >>query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -q t[16-31] works' '
	$powerman -h $testaddr -q t[16-31] >query3.out &&
	makeoutput "" "t[16-31]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'stop powerman daemon' '
        kill -15 $(cat powermand.pid) &&
        wait
'

test_done

# vi: set ft=sh
