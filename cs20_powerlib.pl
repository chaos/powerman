#!/usr/bin/perl
#
# quick and dirty library for cs20 power monitoring and control scripts
# "api_reset" original by
# Jim Garlick 2/15/01
# adapted by
# Andrew Uselton 6/5/01

package CS20Power;

use strict;
use vars qw($tty $wti_passwd $cmd_on $cmd_off $cmd_reset %nodes $node @node_list $ditty $ether $getmac $OK $ERROR $EXITOK $EXITERROR $ON $OFF);

$OK        = 1;
$ERROR     = 0;
$EXITOK    = 0;
$EXITERROR = 1;
$ON        = 1;
$OFF       = 0;

%nodes = (
	"slc6"  => "6",
);

sub cs20_power_init (@node_list)
{
    foreach $node (@node_list) {
	if (!defined($nodes{$node})) {
	    printf STDERR ("cs20_power_init:  $node is invalid - aborting sequence\n");
	    exit($EXITERROR);
	}
    }
    return $OK;
}

sub cs20_power_fin
{
    return $OK;
}

$ether = "ether-wake \%s";
$getmac = "./showdev.exp -j \%s |cut -d\\  -f3 |tr - :";

sub do_reset
{
	my ($node) = @_;
	my $mac;
	my $plug;
	my $ether_com;
	my $mac_com = sprintf $getmac, $node;
	my $ether_val;

	$plug = $nodes{$node};
	$mac = `$mac_com`;
	$ether_com = sprintf $ether, $mac;
	if (CS20Power::plug_on($plug))
	    {
		$ether_val = `$ether_com`;
		$ether_val = `$ether_com`;
		return $ON;
	    }
	return CS20Power::plug_on($plug);
}

sub do_off
{
	my ($node) = @_;
	my $mac;
	my $plug;
	my $ether_com;
	my $mac_com = sprintf $getmac, $node;
	my $ether_val;

	$plug = $nodes{$node};
	$mac = `$mac_com`;
	$ether_com = sprintf $ether, $mac;
	if (CS20Power::plug_on($plug))
	    {
		$ether_val = `$ether_com`;
		return $OFF;
	    }
	return CS20Power::plug_on($plug);
}

sub do_on
{
    my ($node) = @_;
    my $mac;
    my $plug;
    my $ether_com;
    my $mac_com = sprintf $getmac, $node;
    my $ether_val;

    $plug = $nodes{$node};
    $mac = `mac_com`;
    $ether_com = sprintf $ether, $mac;
    if (! CS20Power::plug_on($plug))
    {
	$ether_val = `$ether_com`;
	return $OFF;
    }
    return CS20Power::plug_on($plug);
}

#Note that "node_on" does not require package "init" or "fin"

sub node_on
{
    my ($node) = @_;
    my $plug;

    $plug = $nodes{$node};
    return CS20Power::plug_on($plug);
}

$ditty = "/usr/bin/ditty /dev/ttyD\%s |grep DTR | awk '{print \$5}'\n";

sub plug_on
{
    my $retval;
    my ($plug) = @_;
    my $ditty_com = sprintf $ditty, $plug;
    my $ditty_val = `$ditty_com`;

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
