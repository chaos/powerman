#!/bin/sh

test_description='Test Powerman status query'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11004

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

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
test_expect_success 'powerman -q works' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "t[0-15]" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -q t1 works' '
	$powerman -h $testaddr -q t1 >query2.out &&
	makeoutput "" "t1" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -q t1 t2 works' '
	$powerman -h $testaddr -q t1 t2 >query2a.out &&
	makeoutput "" "t[1-2]" "" >query2a.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -q t[3-5] works' '
	$powerman -h $testaddr -q t[3-5] >query3.out &&
	makeoutput "" "t[3-5]" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_expect_success 'create new powerman.conf with no status_all script' '
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
	    script status {
	        send "stat %s\n"
	        expect "plug ([0-9]+): (ON|OFF)\n"
	        setplugstate \$1 \$2 on="ON" off="OFF"
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
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q works' '
	$powerman -h $testaddr -q >nsa_query.out &&
	makeoutput "" "t[0-15]" "" >nsa_query.exp &&
	test_cmp nsa_query.exp nsa_query.out
'
test_expect_success 'powerman -q t1 works' '
	$powerman -h $testaddr -q t1 >nsa_query2.out &&
	makeoutput "" "t1" "" >nsa_query2.exp &&
	test_cmp nsa_query2.exp nsa_query2.out
'
test_expect_success 'powerman -q t[3-5] works' '
	$powerman -h $testaddr -q t[3-5] >nsa_query3.out &&
	makeoutput "" "t[3-5]" "" >nsa_query3.exp &&
	test_cmp nsa_query3.exp nsa_query3.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand2.pid) &&
	wait
'
test_expect_success 'create test powerman.conf with device over tcp' '
	cat >powerman3.conf <<-EOT
	include "$vpcdev"
	listen "$testaddr"
	device "test0" "vpc" "127.0.0.1:10900"
	node "t[0-15]" "test0"
	EOT
'
test_expect_success 'start device server' '
	$vpcd -p 10900 &
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman3.conf &
	echo $! >powermand3.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d
'
test_expect_success 'powerman -q works' '
	$powerman -h $testaddr -q >tcp_query.out &&
	makeoutput "" "t[0-15]" "" >tcp_query.exp &&
	test_cmp tcp_query.exp tcp_query.out
'
test_expect_success 'powerman -q t1 works' '
	$powerman -h $testaddr -q t1 >tcp_query2.out &&
	makeoutput "" "t1" "" >tcp_query2.exp &&
	test_cmp tcp_query2.exp tcp_query2.out
'
test_expect_success 'powerman -q t[3-5] works' '
	$powerman -h $testaddr -q t[3-5] >tcp_query3.out &&
	makeoutput "" "t[3-5]" "" >tcp_query3.exp &&
	test_cmp tcp_query3.exp tcp_query3.out
'
# powermand will disconnect from device server, which will cause it to exit
test_expect_success 'stop powerman daemon and device server' '
	kill -15 $(cat powermand3.pid) &&
	wait &&
	wait
'
test_expect_success 'create new powerman.conf with both status and status_all script' '
	cat >powerman4.conf <<-EOT4
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
	    script status {
	        send "stat %s\n"
	        expect "plug ([0-9]+): (ON|OFF)\n"
	        setplugstate \$1 \$2 on="ON" off="OFF"
	        expect "[0-9]* OK\n"
	        expect "[0-9]* vpc> "
	    }
	    script status_all {
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
	EOT4
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -c powerman4.conf &
	echo $! >powermand4.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >/dev/null
'
test_expect_success 'powerman -q works' '
	$powerman -h $testaddr -q >all_query.out &&
	makeoutput "" "t[0-15]" "" >all_query.exp &&
	test_cmp all_query.exp all_query.out
'
test_expect_success 'powerman -q uses status_all script on all plugs' '
	$powerman -h $testaddr -T -q >all_queryT.out &&
	count=`grep "stat" all_queryT.out | wc -l` &&
	test $count = 1
'
test_expect_success 'powerman -q t[1-15] works' '
	$powerman -h $testaddr -q t[1-15] >most_query.out &&
	makeoutput "" "t[1-15]" "" >most_query.exp &&
	test_cmp most_query.exp most_query.out
'
test_expect_success 'powerman -q uses status script on not all plugs' '
	$powerman -h $testaddr -T -q t[1-15] >most_queryT.out &&
	count=`grep "stat" most_queryT.out | wc -l` &&
	test $count = 15
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand4.pid) &&
	wait
'

test_done

# vi: set ft=sh
