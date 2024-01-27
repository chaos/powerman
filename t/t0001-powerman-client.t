#!/bin/sh

test_description='Test Powerman Client without server'

. `dirname $0`/sharness.sh

powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman

test_expect_success 'powerman -h fails with usage on stdout' '
	test_must_fail $powerman -h >help.out &&
	grep -q Usage: help.out
'

test_expect_success 'powerman --badopt fails with usage on stdout' '
	test_must_fail $powerman --badopt >badopt.out &&
	grep -q Usage: badopt.out
'
test_done

# vi: set ft=sh
