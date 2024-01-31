#!/bin/sh

test_description='Test Powerman telemetry output'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/../etc/devices/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11005

test_expect_success 'create test powerman.conf' '
	cat >powerman.conf <<-EOT
	include "$vpcdev"
	listen "$testaddr"
	device "test0" "vpc" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf -f &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q -T works' '
	$powerman -h $testaddr -q -T >tel.out
'
test_expect_success 'telemetry output contains send' '
	grep -q "send(test0):" tel.out
'
test_expect_success 'telemetry output contains recv' '
	grep -q "recv(test0):" tel.out
'
test_expect_success 'telemetry has expected size' '
	test $(stat -c%s tel.out) -eq 576
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'

# Note: vpc server has a "spew" command that prints a long line.
# We insert that in the status_all script to increase the volume
# of telemetry data for testing.
generate_config_spew() {
	local spew=$1
	cat <<-EOT
	specification "vpc" {
	    timeout 	5.0
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
	    script status_all {
	        send "spew $spew\n"  # noise generator
	        expect "[0-9]* OK\n"
	        expect "[0-9]* vpc> "
	        send "stat *\n"
	        foreachplug {
	            expect "plug ([0-9]+): (ON|OFF)\n"
	            setplugstate \$1 \$2 on="ON" off="OFF"
	        }
	        expect "[0-9]* OK\n"
	        expect "[0-9]* vpc> "
	    }
        }
	listen "$testaddr"
	device "test0" "vpc" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT
}
test_expect_success 'create test powerman.conf with extra telemetry output < 1K' '
	generate_config_spew 16 >powerman2.conf
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman2.conf -f &
	echo $! >powermand2.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q -T works' '
	$powerman -h $testaddr -q -T >tel2.out
'
test_expect_success 'telemetry has expected size' '
	test $(stat -c%s tel2.out) -eq 1926
'

test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand2.pid) &&
	wait
'
# spew more than 1K (MIN_DEV_BUF) but less than 64K (MAX_DEV_BUF) to
# cause cbuf expansion but no wrap.  (512 * 78 char = 39K)
test_expect_success 'create test powerman.conf with 1K < telemetry output < 64K' '
	generate_config_spew 512 >powerman3.conf
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman3.conf -f &
	echo $! >powermand3.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q -T works' '
	$powerman -h $testaddr -q -T >tel3.out
'
test_expect_success 'telemetry has expected size' '
	test $(stat -c%s tel3.out) -eq 41607
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand3.pid) &&
	wait
'
# spew more than 64K (MAX_DEV_BUF) to cause cbuf wrap.
# (1024 * 78 char = 78K)
test_expect_success 'create test powerman.conf with telemetry output > 64K' '
	generate_config_spew 1024 >powerman4.conf
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman4.conf -f &
	echo $! >powermand4.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q -T works' '
	$powerman -h $testaddr -q -T >tel4.out 2>tel4.err
'
test_expect_success 'telemetry has expected size' '
	test $(stat -c%s tel4.out) -eq 67002
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand4.pid) &&
	wait
'
test_expect_success 'create test powerman.conf with telemetry output > 2*64K' '
	# spew more than twice 64K (MAX_DEV_BUF) to cause double cbuf wrap.
	# (1900 * 78 char = 144K)
	generate_config_spew 1900 >powerman5.conf
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman5.conf -f &
	echo $! >powermand5.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q -T works' '
	$powerman -h $testaddr -q -T >tel5.out 2>tel5.err
'
test_expect_success 'telemetry has expected size' '
	test $(stat -c%s tel5.out) -eq 67002
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand5.pid) &&
	wait
'

test_done

# vi: set ft=sh
