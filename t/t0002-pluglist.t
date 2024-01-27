#!/bin/sh

test_description='Test pluglist class'

. `dirname $0`/sharness.sh

test_pluglist=$SHARNESS_BUILD_DIRECTORY/src/powerman/test_pluglist

test_expect_success 'pluglist foo[1-500] p[1-500] maps expected host to p493' "
	$test_pluglist -f p493 'foo[1-500]' 'p[1-500]' >find.out &&
	cat >find.exp <<-EOT
	plug=p493 node=foo493
	EOT
"

test_expect_success 'pluglist t[1,2,3] p[999-1001] has expected map' "
	$test_pluglist 't[1,2,3]' 'p[999-1001]' >map1.out &&
	cat >map1.exp <<-EOT &&
	plug=p1001 node=t3
	plug=p1000 node=t2
	plug=p999 node=t1
	EOT
	test_cmp map1.exp map1.out
"

test_expect_success 'pluglist foo[11-15] [1-5] correctly maps 1,2,3,4,5,6' "
	$test_pluglist -p1,2,3,4,5,6 'foo[11-15]' '[1-5]' >map2.out &&
	cat >map2.exp <<-EOT &&
	plug=1 node=foo11
	plug=2 node=foo12
	plug=3 node=foo13
	plug=4 node=foo14
	plug=5 node=foo15
	plug=6 node=NULL
	EOT
	test_cmp map2.exp map2.out
"

test_expect_success 'pluglist foo[1-500] [1-500] correctly maps 1,2,3,4,5,6,7,8' "
	$test_pluglist -p 1,2,3,4,5,6,7,8 'foo[1-500]' '[1-500]' >map3.out \
		2>map3.err &&
	cat >map3.exp <<-EOT &&
	plug=1 node=foo1
	plug=2 node=foo2
	plug=3 node=foo3
	plug=4 node=foo4
	plug=5 node=foo5
	plug=6 node=foo6
	plug=7 node=foo7
	plug=8 node=foo8
	EOT
	test_cmp map3.exp map3.out &&
	grep -q 'unknown plug' map3.err
"

test_done

# vi: set ft=sh
