#!/usr/bin/perl
#
# quick and dirty library for up2000 power monitoring and control scripts
# "api_reset" original by
# Jim Garlick 2/15/01
# adapted by
# Andrew Uselton 6/5/01

package UP2000Power;

use strict;
use vars qw($tty $wti_passwd $cmd_on $cmd_off $cmd_reset %nodes $node @node_list $ditty $ditty_com $ditty_val $OK $ERROR $EXITOK $EXITERROR $ON $OFF);

$tty = "/dev/ttyD23";

$wti_passwd = "";
$cmd_on 	= "1";
$cmd_off 	= "0";
$cmd_reset 	= "T";

$OK        = 1;
$ERROR     = 0;
$EXITOK    = 0;
$EXITERROR = 1;
$ON        = 1;
$OFF       = 0;

%nodes = (
	"slc0"  => "0",
	"slc1"  => "1",
	"slc2" => "2",
	"slc3" => "3",
	"slc4" => "4",
	"slc5" => "5",
);

sub up2000_power_init (@node_list)
{
    if (!open(LINE, "+> $tty")) {
	printf STDERR ("Could not open $tty: $!\n");
	exit($EXITERROR);
    }
    foreach $node (@node_list) {
	if (!defined($nodes{$node})) {
	    printf STDERR ("up2000_power_init:  $node is invalid - aborting sequence\n");
	    exit($EXITERROR);
	}
    }
    return $OK;
}

sub up2000_power_fin
{
    close (LINE);
    return $OK;
}

sub do_reset
{
	my ($node) = @_;
	my $plug;

	$plug = $nodes{$node};
	if (UP2000Power::plug_on($plug))
	    {
		printf LINE ("%s%s%s\r\n", $wti_passwd, $plug, $cmd_reset);
		while (<LINE>) {
		    last if (/Complete/);
		}
	    }
	return UP2000Power::plug_on($plug);
}

sub do_off
{
	my ($node) = @_;
	my $plug;

	$plug = $nodes{$node};
	if (UP2000Power::plug_on($plug))
	    {
		printf LINE ("%s%s%s\r\n", $wti_passwd, $plug, $cmd_off);
		while (<LINE>) {
		    last if (/Complete/);
		}
		return $OFF;
	    }
	return UP2000Power::plug_on($plug);
}

sub do_on
{
    my ($node) = @_;
	my $plug;

	$plug = $nodes{$node};
	if (! UP2000Power::plug_on($plug))
	    {
		printf LINE ("%s%s%s\r\n", $wti_passwd, $plug, $cmd_on);
		while (<LINE>) {
		    last if (/Complete/);
		}
		return $OFF;
	    }
	return UP2000Power::plug_on($plug);
}

#Note that "node_on" does not require package "init" or "fin"

sub node_on
{
    my ($node) = @_;
    my $plug;

    $plug = $nodes{$node};
    return UP2000Power::plug_on($plug);
}

$ditty = "/usr/bin/ditty /dev/ttyD\%s |grep DTR | awk '{print \$5}'\n";

sub plug_on
{
    my $retval;
    my ($plug) = @_;

    $ditty_com = sprintf $ditty, $plug;
    $ditty_val = `$ditty_com`;
    chomp $ditty_val;
    if($ditty_val eq "DSR+")
    {
	$retval = $ON;	
    }
    else
    {
	$retval = $OFF;
    }
# note that we're returning 1 for "True" or "Is on" in the usual C
# programming convention where 0 == "False"
    return $retval;
}

# So there's a return value
1;
