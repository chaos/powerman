#!/usr/bin/perl
#
# quick and dirty library for es40 power monitoring and control scripts
# "api_reset" original by
# Jim Garlick 2/15/01
# adapted by
# Andrew Uselton 6/14/01

package ES40Power;

use strict;
use vars qw(%nodes $node @node_list $ditty $ditty_com $ditty_val $OK $ERROR $EXITOK $EXITERROR $ON $OFF);

$OK        = 1;
$ERROR     = 0;
$EXITOK    = 0;
$EXITERROR = 1;
$ON        = 1;
$OFF       = 0;

%nodes = (
	  "slc8"  => "8",
	  "slc9"  => "9",
	  "slc10" => "10",
	  "slc11" => "11",
	  "slc12" => "12",
	  "slc13" => "13",
	  "slc14" => "14",
	  "slc15" => "15"
);

sub es40_power_init (@node_list)
{
    foreach $node (@node_list) {
	if (!defined($nodes{$node})) {
	    printf STDERR ("es40_power_init:  $node is invalid - aborting sequence\n");
	    exit($EXITERROR);
	}
    }
    return $OK;
}

sub es40_power_fin
{
    return $OK;
}

sub do_reset
{
    my ($node) = @_;
    my $plug;
    
    $plug = $nodes{$node};
    if (ES40Power::plug_on($plug))
    {
	expect_reset();
    }
    return ES40Power::plug_on($plug);
}

sub do_off
{
    my ($node) = @_;
    my $plug;
    
    $plug = $nodes{$node};
    if (ES40Power::plug_on($plug))
    {
	expect_off();
    }
    return ES40Power::plug_on($plug);
}

sub do_on
{
    my ($node) = @_;
    my $plug;

    $plug = $nodes{$node};
    if (! ES40Power::plug_on($plug))
    {
	expect_on();
    }
    return ES40Power::plug_on($plug);
}

sub expect_reset
{
    return $OK;
}

sub expect_on
{
    return $OK;
}

sub expect_off
{
    return $OK;
}

$ditty = "/usr/bin/ditty /dev/ttyD\%s |grep DTR | awk '{print \$5}'\n";

sub node_on
{
    my ($node) = @_;
    my $plug;

    $plug = $nodes{$node};
    return ES40Power::plug_on($plug);
}

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
