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

test_expect_success 'powerman without an action fails with message on stderr' '
	test_must_fail $powerman -T 2>noaction.err &&
	grep "No action was specified" noaction.err
'

test_expect_success 'powerman --version works' '
	$powerman --version >version.out &&
	test $(wc -l <version.out) -eq 1
'
test_expect_success 'powerman --license works' '
	$powerman --license >license.out &&
	grep GPL license.out
'

test_done

# vi: set ft=sh
