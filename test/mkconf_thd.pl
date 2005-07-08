#!/usr/bin/perl
#
# $Id$
#
$maxnodes = $ARGV[0];
$baseport = 8080;
$nodespervpc = 16;
$devs = $maxnodes / $nodespervpc;
$pwd = `pwd`; chomp $pwd;

print "include \"../etc/vpc.dev\"\n";
print "port 10103  # std port + 2\n";
print "tcpwrappers no\n";

for ($i = 0; $i < $devs; $i++) {
	$m = $i * $nodespervpc;
	$n = $m + $nodespervpc - 1;
	$p = $i + $baseport;
	#print "device \"test$i\" \"vpc\" \"$pwd/vpcd -f |&\"\n";
	print "device \"test$i\" \"vpc\" \"localhost:$p\"\n";
	print "node \"t[$m-$n]\" \"test$i\"\n";
}

exit 0;
