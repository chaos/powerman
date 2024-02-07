#!/bin/sh

test_description='Check LLNL El Capitan config'

. `dirname $0`/sharness.sh

ulimit -n 2048

powermand=$SHARNESS_BUILD_DIRECTORY/src/powerman/powermand
powerman=$SHARNESS_BUILD_DIRECTORY/src/powerman/powerman
simdir=$SHARNESS_BUILD_DIRECTORY/t/simulators
devicesdir=$SHARNESS_TEST_SRCDIR/../etc/devices

# Use port = 11000 + test number
# That way there won't be port conflicts with make -j
testaddr=localhost:11034

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
		echo "device \"cmm$i\" \"redfishpower-cray-olympus\" \"$simdir/redfishpower -h elcap-cmm$i,pelcap[$lo-$hi] |&\""
	done
}
# Usage: gennodes_long chassis_count
gennodes_long() {
	echo "### CMM Chassis"
	for i in $(seq 0 $(($1-1))); do
		echo "node \"elcap-cmm$i\" \"cmm$i\" \"Enclosure\""
	done

	echo "### CMM Blades"
	for i in $(seq 0 $(($1-1))); do
		lo=$(($i*8))
		hi=$((${lo}+7))
		echo "node \"elcap-blade[$lo-$hi]\" \"cmm$i\" \"Blade[0-7]\""
	done
	echo "### CMM Perif"
	for i in $(seq 0 $(($1-1))); do
		lo=$(($i*8))
		hi=$((${lo}+7))
		echo "node \"elcap-perif[$lo-$hi]\" \"cmm$i\" \"Perif[0-7]\""
	done
	echo "### Nodes"
	for i in $(seq 0 $(($1-1))); do
		lo=$(($i*16))
		hi=$((${lo}+15))
		echo "node \"elcap[$lo-$hi]\" \"cmm$i\" \"Node[0-15]\""
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
	echo "include \"$devicesdir/redfishpower-cray-olympus.dev\""; \
	gendevices 1024; \
	gennodes 1024) >powerman.conf
'
test_expect_success 'start powerman daemon and wait for it to start' '
	$powermand -Y -c powerman.conf &
	echo $! >powermand.pid &&
	$powerman --retry-connect=100 --server-host=$testaddr -d >device.out
'
ALLSTR="elcap[0-16383],elcap-blade[0-8191],elcap-cmm[0-1023],elcap-perif[0-8191]"
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query.out &&
	makeoutput "" "$ALLSTR" "" >query.exp &&
	test_cmp query.exp query.out
'
test_expect_success 'powerman -1 all works' '
	$powerman -h $testaddr -1 $ALLSTR >on.out &&
	is_successful on.out
'
test_expect_success 'powerman -q shows all on' '
	$powerman -h $testaddr -q >query2.out &&
	makeoutput "$ALLSTR" "" "" >query2.exp &&
	test_cmp query2.exp query2.out
'
test_expect_success 'powerman -0 all works' '
	$powerman -h $testaddr -0 $ALLSTR >off.out &&
	is_successful off.out
'
test_expect_success 'powerman -q shows all off' '
	$powerman -h $testaddr -q >query3.out &&
	makeoutput "" "$ALLSTR" "" >query3.exp &&
	test_cmp query3.exp query3.out
'
test_expect_success 'stop powerman daemon' '
	kill -15 $(cat powermand.pid) &&
	wait
'
test_done

# vi: set ft=sh
