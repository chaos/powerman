#!/bin/sh

test_description='Test Powerman list query'

. `dirname $0`/sharness.sh

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
vpcd=$SHARNESS_BUILD_DIRECTORY/t/simulators/vpcd
vpcdev=$SHARNESS_TEST_SRCDIR/etc/vpc.dev

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11033

if ! valgrind --version; then
	skip_all='skipping valgrind tests'
	test_done
fi

test_expect_success 'create test powerman.conf' '
	cat >powerman.conf <<-EOT
	include "$vpcdev"
	listen "$testaddr"
	device "test0" "vpc" "$vpcd |&"
	node "t[0-15]" "test0"
	alias "a0" "t0"
	EOT
'
test_expect_failure 'run powermand --stdio under valgrind' '
	valgrind --tool=memcheck --leak-check=full --error-exitcode=1 \
	    $powermand --stdio -c powerman.conf 2>valgrind.err <<-EOT
	help
	EOT
'
test_done

# vi: set ft=sh
