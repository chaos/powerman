#!/bin/sh

test_description='Test Powerman Client without server'

. `dirname $0`/sharness.sh

powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman

test_expect_success 'powerman --help displays usage' '
	$powerman --help >help.out &&
	grep Usage: help.out
'

test_expect_success 'powerman --badopt fails with error on stderr' '
	test_must_fail $powerman --badopt 2>badopt.out &&
	grep "Unknown option" badopt.out
'

test_done

# vi: set ft=sh
