#!/usr/bin/perl
#
# $Id$
#
$maxnodes = $ARGV[0];
$devpath = $ARGV[1];
$nodespervpc = 16;
$devs = $maxnodes / $nodespervpc;

print "include \"$devpath/vpc.dev\"\n";
for ($i = 0; $i < $devs; $i++) {
	$m = $i * $nodespervpc;
	$n = $m + $nodespervpc - 1;
	print "device \"test$i\" \"vpc\" \"./vpcd -f |&\"\n";
	print "node \"t[$m-$n]\" \"test$i\"\n";
}

exit 0;
