#!/usr/bin/perl
#
# Library for calling power monitoring and control functions across
# node types.  The genders "type" attribute/value pair produces the node
# type hash used here.
# Andrew Uselton 6/15/01
#

package Power;

use strict;

# might want to put a path in here
require "es40_powerlib.pl";
require "cs20_powerlib.pl";
require "up2000_powerlib.pl";

use vars qw($node $val %types $type %need %stat $OK $ERROR $EXITOK $EXITERROR 
	    $ON $OFF $UNASKED $ASKED @cluster);

$OK        = 1;
$ERROR     = 0;
$EXITOK    = 0;
$EXITERROR = 1;
$ON        = 1;
$OFF       = 0;
$UNASKED   = -1;
$ASKED     = 2;

@cluster = 
(
    "slc0",  "slc1",  "slc2",  "slc3", 
    "slc4",  "slc5",  "slc6",  
    "slc8",  "slc9",  "slc10", "slc11", 
    "slc12", "slc13", "slc14", "slc15"
);

%types = 
(
 "slc0"  => "up2000",
 "slc1"  => "up2000",
 "slc2"  => "up2000",
 "slc3"  => "up2000",
 "slc4"  => "up2000",
 "slc5"  => "up2000",
 "slc6"  => "cs20",
# There is currently no slc7 as it was a borrow that was returned.
 "slc8"  => "es40",
 "slc9"  => "es40",
 "slc10" => "es40",
 "slc11" => "es40",
 "slc12" => "es40",
 "slc13" => "es40",
 "slc14" => "es40",
 "slc15" => "es40"
);

%stat = 
(
 "slc0"  => $UNASKED,
 "slc1"  => $UNASKED,
 "slc2"  => $UNASKED,
 "slc3"  => $UNASKED,
 "slc4"  => $UNASKED,
 "slc5"  => $UNASKED,
 "slc6"  => $UNASKED,
 "slc8"  => $UNASKED,
 "slc9"  => $UNASKED,
 "slc10" => $UNASKED,
 "slc11" => $UNASKED,
 "slc12" => $UNASKED,
 "slc13" => $UNASKED,
 "slc14" => $UNASKED,
 "slc15" => $UNASKED
);

%need =
(
  "up2000" => "no",
  "cs20"   => "no",
  "es40"   => "no"
 );

sub power_init(@ARGV)
{
    my @up2000_init = ();
    my @cs20_init   = ();
    my @es40_init   = ();
    
    stat_init();
    foreach $node (@ARGV) 
    {
	if (!defined($types{$node})) 
	{
	    printf STDERR ("power_init: $node is invalid - aborting sequence\n");
	    exit($EXITERROR);
	}
	$type = $types{$node};
	
	if ($type eq "up2000")
	{
	    $need{"up2000"} = "yes";
	    @up2000_init = (@up2000_init, $node);
	}
	elsif ($type eq "cs20")
	{
	    $need{"cs20"} = "yes";
	    @cs20_init = (@cs20_init, $node);
	}
	elsif ($type eq "es40")
	{
	    $need{"es40"} = "yes";
	    @es40_init = (@es40_init, $node);
	}
	else
	{
	    printf STDERR ("$node has invalid type $type - aborting sequence\n");
	    exit($EXITERROR);	
	}
    }
    if($need{"up2000"} eq "yes")
    {
      UP2000Power::up2000_power_init(@up2000_init);
    }
    if($need{"es40"} eq "yes")
    {
      CS20Power::cs20_power_init(@cs20_init);
    }
    if($need{"es40"} eq "yes")
    {
      ES40Power::es40_power_init(@es40_init);
    }
}

sub stat_init
{
    foreach $node (@cluster)
    {
	$stat{$node} = $UNASKED;
    }
}

sub power_fin
{
    if($need{"up2000"} eq "yes")
    {
      UP2000Power::up2000_power_fin();
    }
    if($need{"cs20"} eq "yes")
    {
      CS20Power::cs20_power_fin();
    }
    if($need{"es40"} eq "yes")
    {
      ES40Power::es40_power_fin();
    }
}

sub power_cluster
{
    return @cluster;
}

sub power_stat(@ARGV)
{
    stat_init();
    foreach $node (@ARGV)
    {
	if(!defined($stat{$node}))
	{
	    printf STDERR ("power_stat: $node is invalid - aborting sequence\n");
	    exit($EXITERROR);
	}
	$stat{$node} = Power::node_on($node);
    }     
    return %stat;
}

sub do_reset(@ARGV)
{
    my $retval;

    $retval = Power::nodes_on(@ARGV);
    if ($retval == $ON)
    {
	foreach $node (@ARGV) 
	{
	    $type = $types{$node};
	    
	    if ($type eq "up2000")
	    {
	      UP2000Power::do_reset($node);
	    }
	    elsif ($type eq "cs20")
	    {
	      CS20Power::do_reset($node);
	    }
	    elsif ($type eq "es40")
	    {
	      ES40Power::do_reset($node);
	    }
	}
    }
    return $retval;
}

sub do_on(@ARGV)
{
    my $retval;

    $retval = Power::nodes_off(@ARGV);
    if ($retval == $OFF)
    {
	foreach $node (@ARGV) 
	{
	    $type = $types{$node};
	    
	    if ($type eq "up2000")
	    {
	      UP2000Power::do_on($node);
	    }
	    elsif ($type eq "cs20")
	    {
	      CS20Power::do_on($node);
	    }
	    elsif ($type eq "es40")
	    {
	      ES40Power::do_on($node);
	    }
	}
    }
    return $retval;
}

sub do_off(@ARGV)
{
    my $retval;

    $retval = Power::nodes_on(@ARGV);
    if ($retval == $ON)
    {
	foreach $node (@ARGV) 
	{
	    $type = $types{$node};
	    
	    if ($type eq "up2000")
	    {
	      UP2000Power::do_off($node);
	    }
	    elsif ($type eq "cs20")
	    {
	      CS20Power::do_off($node);
	    }
	    elsif ($type eq "es40")
	    {
	      ES40Power::do_off($node);
	    }
	}
    }
    return $retval;
}

sub nodes_on(@ARGV)
{
    my $retval = $ON;
    my $checkval;

    foreach $node (@ARGV) 
    {
	$type = $types{$node};

	if ($type eq "up2000")
	{
	  $checkval = UP2000Power::node_on($node);
	}
	elsif ($type eq "cs20")
	{
	  $checkval = CS20Power::node_on($node);
	}
	elsif ($type eq "es40")
	{
	  $checkval = ES40Power::node_on($node);
	}
	if ($checkval == $OFF)
	{
	    $retval = $OFF;
	}
    }
    return $retval;
}

sub nodes_off(@ARGV)
{
    my $retval = $OFF;
    my $checkval;

    foreach $node (@ARGV) 
    {
	$type = $types{$node};

	if ($type eq "up2000")
	{
	  $checkval = UP2000Power::node_on($node);
	}
	elsif ($type eq "cs20")
	{
	  $checkval = CS20Power::node_on($node);
	}
	elsif ($type eq "es40")
	{
	  $checkval = ES40Power::node_on($node);
	}
	if ($checkval == $ON)
	{
	    $retval = $ON;
	}
    }
    return $retval;
}

sub node_on($node)
{
    my $retval;

    $type = $types{$node};
    
    if ($type eq "up2000")
    {
	$retval = UP2000Power::node_on($node);
    }
    elsif ($type eq "cs20")
    {
	$retval = CS20Power::node_on($node);
    }
    elsif ($type eq "es40")
    {
	$retval = ES40Power::node_on($node);
    }
    return $retval;
}

# So there's a return value
1;

