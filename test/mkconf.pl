#!/usr/bin/perl
#
# $Id$
#
$maxnodes = $ARGV[0];
$nodespervpc = 16;
$devs = $maxnodes / $nodespervpc;
$pwd = `pwd`; chomp $pwd;

print "include \"../etc/vpc.dev\"\n";
print "port 10102  # std port + 1\n";
print "tcpwrappers no\n";

for ($i = 0; $i < $devs; $i++) {
	$m = $i * $nodespervpc;
	$n = $m + $nodespervpc - 1;
	print "device \"test$i\" \"vpc\" \"$pwd/vpcd -f |&\"\n";
	print "node \"t[$m-$n]\" \"test$i\"\n";
}

exit 0;
