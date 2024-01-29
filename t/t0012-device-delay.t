#!/bin/sh

test_description='Ensure that delays are processed expeditiously'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/test/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/../etc/devices/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11012

test_expect_success 'create test powerman.conf with cycle of only delay' '
	cat >powerman.conf <<-EOT
	specification "vpcfakecycle" {
	    timeout	2
	    plug name { "0" "1" "2" "3" "4" "5" "6" "7" "8"
	                "9" "10" "11" "12" "13" "14" "15" }
	    script login {
	        send "login\n"
	        expect "[0-9]* OK\n"
	        expect "[0-9]* vpc> "
	    }
	    script cycle {
	        delay 0.01
	    }
	}
	listen "$testaddr"
	device "test0" "vpcfakecycle" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'run powerman -c t1 128x in a row' '
	for i in $(seq 0 127); do \
		$powerman -h $testaddr -c t1; \
	done >cycle.out &&
	for i in $(seq 0 127); do \
		echo Command completed successfully
	done >cycle.exp &&
	test_cmp cycle.exp cycle.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_expect_success 'create test powerman.conf with cycle of delay + io' '
	cat >powerman2.conf <<-EOT
	specification "vpcfakecycle2" {
	    timeout	2
	    plug name { "0" "1" "2" "3" "4" "5" "6" "7" "8"
	                "9" "10" "11" "12" "13" "14" "15" }
	    script login {
	        send "login\n"
	        expect "[0-9]* OK\n"
	        expect "[0-9]* vpc> "
	    }
	    script cycle {
	        delay 0.01
	        send "\n"
	        expect "[0-9]* vpc> "
	    }
	}
	listen "$testaddr"
	device "test0" "vpcfakecycle2" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman2.conf -f &
	echo $! >powermand2.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'run powerman -c t1 128x in a row' '
	for i in $(seq 0 127); do \
		$powerman -h $testaddr -c t1; \
	done >cycle2.out &&
	test_cmp cycle.exp cycle2.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand2.pid) &&
	wait
'

test_done

# vi: set ft=sh
