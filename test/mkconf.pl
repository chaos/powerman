#!/usr/bin/perl
#
# Usage: mkconf.pl plugcount
#
# Builds a powerman.conf file for a vpc config for plugcount plugs.

$maxnodes = $ARGV[0];
$nodespervpc = 16;
$devs = $maxnodes / $nodespervpc;

print "include \"vpc.dev\"\n";
for ($i = 0; $i < $devs; $i++) {
	$m = $i * $nodespervpc;
	$n = $m + $nodespervpc - 1;
	print "device \"test$i\" \"vpc\" \"./vpcd |&\"\n";
	print "node \"t[$m-$n]\" \"test$i\"\n";
}

exit 0;
