#!/usr/bin/perl
#
# $Id$
# $Source$
#
$maxnodes = $ARGV[0];
$nodespervpc = 8;
$devs = $maxnodes / $nodespervpc;

print "include \"../etc/vpc.dev\"\n";
print "port 10102  # std port + 1\n";
print "tcpwrappers no\n";

for ($i = 0; $i < $devs; $i++) {
	$m = $i * $nodespervpc;
	$n = $m + $nodespervpc - 1;
	print "device \"test$i\" \"vpc\" \"./vpcd -f |&\"\n";
	print "node \"t[$m-$n]\" \"test$i\"\n";
}

exit 0;
