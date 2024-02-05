#!/bin/sh

test_description='Check that fixes to issues stay fixed'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices
simdir=$SHARNESS_BUILD_DIRECTORY/t/simulators
vpcd=$simdir/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11028


makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}

# https://code.google.com/archive/p/powerman/issues/3
test_expect_success 'gc#3: make bad config to check error has correct line no' '
	cat >powerman_gc3.dev<<-EOT &&
	# 1
	# 2
	# 3
	# 4
	syntaxerror # 5
	# 6
	# 7
	EOT
	cat >powerman_gc3.conf <<-EOT2
	# comment
	include "$vpcdev"
	# comment
	include "powerman_gc3.dev"
	EOT2
'
test_expect_success 'gc#3: correct line number is called out' '
	test_must_fail $powermand -Y -c powerman_gc3.conf 2>gc3.err &&
	grep "parse error: powerman_gc3.dev::5" gc3.err
'

# https://code.google.com/archive/p/powerman/issues/34
test_expect_success 'gc#34: create config that triggered duplicate node name' '
	cat >powerman_gc34.conf <<-EOT
	listen "$testaddr"
	include "$vpcdev"
	device "linkfarm" "vpc" "$vpcd |&"
	device "mds" "vpc" "$vpcd |&"
	node "widow-mds" "linkfarm" "1"
	node "widow-mds[0-3]" "mds" "[2-5]"
	EOT
'
test_expect_success 'gc#34: powermand starts ok' '
	$powermand -Y -c powerman_gc34.conf &
	echo $! >powermand_gc34.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q
'
test_expect_success 'gc#34: stop powerman daemon' '
	kill -15 $(cat powermand_gc34.pid) &&
	wait
'

# https://code.google.com/archive/p/powerman/issues/35
test_expect_success 'gc#35: create another config that triggered duplicate node name' '
	cat >powerman_gc35.conf <<-EOT
	listen "$testaddr"
	include "$vpcdev"
	device "pdu1" "vpc" "$vpcd |&"
	node "corn1-p2" "pdu1" "1"
	node "corn2" "pdu1" "2"
	EOT
'
test_expect_success 'gc#35: powermand starts ok' '
	$powermand -Y -c powerman_gc35.conf &
	echo $! >powermand_gc35.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q
'
test_expect_success 'gc#35: stop powerman daemon' '
	kill -15 $(cat powermand_gc35.pid) &&
	wait
'

# chaos/powerman#14
# N.B. --auth-failure will make the first node report "password verification
# timeout", which should lead to unknown state
test_expect_success 'issue#14: create config for ipmipower auth parse error' '
	cat >powerman_issue14.conf <<-EOT
	listen "$testaddr"
	include "$devicesdir/ipmipower.dev"
	device "d0" "ipmipower" "$simdir/ipmipower --auth-failure -h t[0-15] |&"
	node "t[0-15]" "d0"
	EOT
'
test_expect_success 'issue#14: powermand starts ok and answers query' '
	$powermand -Y -c powerman_issue14.conf &
	echo $! >powermand_issue14.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -q >issue14.out
'
test_expect_success 'issue#14: power state of auth failure node is unknown' '
	makeoutput "" "t[1-15]" "t0" >issue14.exp &&
	test_cmp issue14.exp issue14.out
'
test_expect_success 'issue#14: stop powerman daemon' '
	kill -15 $(cat powermand_issue14.pid) &&
	wait
'

test_done

# vi: set ft=sh
