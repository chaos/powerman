#!/bin/sh

test_description='Test Powerman temperature query'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11007

test_expect_success 'create test powerman.conf' '
	cat >powerman.conf <<-EOT
	include "$vpcdev"
	listen "$testaddr"
	device "test0" "vpc" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -t works' '
	$powerman -h $testaddr -t >query_all.out &&
	cat >query_all.exp <<-EOT &&
	t0: 83
	t1: 84
	t2: 85
	t3: 86
	t4: 87
	t5: 88
	t6: 89
	t7: 90
	t8: 91
	t9: 92
	t10: 93
	t11: 94
	t12: 95
	t13: 96
	t14: 97
	t15: 98
	EOT
	test_cmp query_all.exp query_all.out
'
test_expect_success 'powerman -t t13 works' '
	$powerman -h $testaddr -t t13 >query_t13.out &&
	echo "t13: 96" >query_t13.exp &&
	test_cmp query_t13.exp query_t13.out
'
test_expect_success 'powerman -t t[13-15] works' '
	$powerman -h $testaddr -t t[13-15] >query_set.out &&
	cat >query_set.exp <<-EOT &&
	t13: 96
	t14: 97
	t15: 98
	EOT
	test_cmp query_set.exp query_set.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_expect_success 'create new powerman.conf with both status_temp and status_temp_all script' '
	cat >powerman2.conf <<-EOT2
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
	    script status_temp {
	        send "temp %s\n"
	        expect "plug ([0-9]+): ([0-9]+)\n"
	        setplugstate \$1 \$2
	        expect "[0-9]* OK\n"
	        expect "[0-9]* vpc> "
	    }
	    script status_temp_all {
		send "temp *\n"
		foreachplug {
			expect "plug ([0-9]+): ([0-9]+)\n"
			setplugstate \$1 \$2
		}
		expect "[0-9]* OK\n"
		expect "[0-9]* vpc> "
	    }
	}
	listen "$testaddr"
	device "test0" "vpc" "$vpcd |&"
	node "t[0-15]" "test0"
	EOT2
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman2.conf &
	echo $! >powermand2.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -t >/dev/null
'
test_expect_success 'powerman -t works' '
	$powerman -h $testaddr -t >all_query.out &&
	cat >all_query.exp <<-EOT &&
	t0: 83
	t1: 84
	t2: 85
	t3: 86
	t4: 87
	t5: 88
	t6: 89
	t7: 90
	t8: 91
	t9: 92
	t10: 93
	t11: 94
	t12: 95
	t13: 96
	t14: 97
	t15: 98
	EOT
	test_cmp all_query.exp all_query.out
'
test_expect_success 'powerman -t uses temp_status_all script on all plugs' '
	$powerman -h $testaddr -T -t >all_queryT.out &&
	count=`grep "temp" all_queryT.out | wc -l` &&
	test $count = 1
'
test_expect_success 'powerman -t t[1-15] works' '
	$powerman -h $testaddr -t t[1-15] >most_query.out &&
	cat >most_query.exp <<-EOT &&
	t1: 84
	t2: 85
	t3: 86
	t4: 87
	t5: 88
	t6: 89
	t7: 90
	t8: 91
	t9: 92
	t10: 93
	t11: 94
	t12: 95
	t13: 96
	t14: 97
	t15: 98
	EOT
	test_cmp most_query.exp most_query.out
'
test_expect_success 'powerman -t uses temp_status script on not all plugs' '
	$powerman -h $testaddr -T -t t[1-15] >most_queryT.out &&
	count=`grep "temp" most_queryT.out | wc -l` &&
	test $count = 15
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand2.pid) &&
	wait
'

test_done

# vi: set ft=sh
