#!/bin/sh

test_description='Check LLNL El Capitan config'

. `dirname $0`/sharness.sh

ulimit -n 2048

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
redfishpower=$SHARNESS_BUILD_DIRECTORY/src/redfishpower/redfishpower
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11039

echo "Command completed successfully" >success.exp

is_successful() {
	test_cmp success.exp $1
}

makeoutput() {
	printf "on:      %s\n" $1
	printf "off:     %s\n" $2
	printf "unknown: %s\n" $3
}


# Usage: gendevices chassis_count
gendevices() {
	for i in $(seq 0 $(($1-1))); do
		lo=$(($i*16))
		hi=$((${lo}+15))
		echo "device \"cmm$i\" \"cray-ex\" \"$redfishpower --test-mode -h elcap-cmm$i,pelcap[$lo-$hi] |&\""
	done
}
# Usage: gennodes chassis_count
gennodes() {
	for i in $(seq 0 $(($1-1))); do
		lo=$(($i*8))
		hi=$((${lo}+7))
		nlo=$(($i*16))
		nhi=$((${nlo}+15))
		echo "node \"elcap-cmm$i,elcap-perif[$lo-$hi],elcap-blade[$lo-$hi],elcap[$nlo-$nhi]\" \"cmm$i\""
	done
}

# This is just a placeholder until we get the config of the real
# HPE Cray EX Shasta system
test_expect_success 'create powerman.conf for El Cap' '
	(echo "listen \"$testaddr\""; \
	echo "include \"$devicesdir/redfishpower-cray-ex.dev\""; \
	gendevices 1024; \
	gennodes 1024) >powerman.conf
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d >device.out
'
CMMSTR="elcap-cmm[0-1023]"
BLADESTR="elcap-blade[0-8191]"
PERIFSTR="elcap-perif[0-8191]"
NODESTR="elcap[0-16383]"
ALLSTR="$NODESTR,$BLADESTR,$CMMSTR,$PERIFSTR"
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "$ALLSTR" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman can turn on Enlcosures' '
	$powerman -h $testaddr -1 $CMMSTR >on.out &&
	is_successful on.out
'
test_expect_success 'powerman -q shows Enclosures on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "$CMMSTR" "$NODESTR,$BLADESTR,$PERIFSTR" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman can turn on Blades + Perifs' '
	$powerman -h $testaddr -1 $BLADESTR,$PERIFSTR >on2.out &&
	is_successful on2.out
'
test_expect_success 'powerman -q shows Enclosures + Blades + Perifs on' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "$BLADESTR,$CMMSTR,$PERIFSTR" "$NODESTR" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'powerman can turn on Nodes' '
	$powerman -h $testaddr -1 $NODESTR >on3.out &&
	is_successful on3.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query4.out &&
	makeoutput "$ALLSTR" "" "" >query4.exp &&
	test_cmp query4.exp query4.out
'
test_expect_success 'powerman -0 all works' '
	$powerman -h $testaddr -0 $ALLSTR >off.out &&
	is_successful off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query5.out &&
	makeoutput "" "$ALLSTR" "" >query5.exp &&
	test_cmp query5.exp query5.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
