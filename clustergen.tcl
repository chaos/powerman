#!/usr/bin/tclsh

set size  0
set racks 1
set name  ""
for {set i 0} {$i < $argc} {incr i} {
    set arg [lindex $argv $i]
    if {[scan $arg "%d" num]} {
	if {$size == 0} {set size $num} else {set racks $num}
    } else {
	scan $arg "%s" name
    }
}

if {$size == 0} {
    puts "I didn\'t get a size out of that"
    exit
}

if {[string length $name] == 0} {
    puts "I didn\'t get a name out of that"
    exit
}

set per [expr ceil($size/double($racks))]

set l "    {%s%d rack%d {%s_power %d} {%s_check %d} {%s_on.gif %s_off.gif}}"

set line(0) "#################################################################"
set line(1) "# \$Id$"
set line(2) "# by Andrew C. Uselton <uselton2@llnl.gov> "
set line(3) "# Copyright (C) 2000 Regents of the University of California"
set line(4) "# See ../DISCLAIMER"
set line(5) "# v. 0-0-3:  2001-08-08"
set line(6) "#            (first version of clustergen.tcl in powerman)"
set line(7) "####################################################################"
set line(8) "# This is the cluster configuration file for the \"powerlib\" library."
set line(9) "# It consists entirely of a tcl expression setting the \"cluster\" "
set line(10) "# variable, which describes the cluster with one entry per node.  An"
set line(11) "# entry consists of the tuple {<name> <location> <power> <check> <gifs>}, "
set line(12) "# where <name> is the name of the node, <location> is some string "
set line(13) "# possibly reflecting which rack the node is in, <power> is the power "
set line(14) "# control type, <check> is the power monitoring type, and <gifs> gives"
set line(15) "# the set of pictures (if any) to display for states of the node.  "
set line(16) "#"
set line(17) "# This file was automatically generated with \"clustergen.tcl\"."
set line(18) "set cluster {"
set header 19
set rack 0
set in_rack 0
for {set i 0} {$i < $size} {incr i} {
    set line([expr $i + $header]) [format $l $name $i $rack $name $i $name $i $name $name]
    incr in_rack
    if {$in_rack >= $per} {
	incr rack
	set  in_rack 0
    }
}
set line([expr $size + $header]) "}"

for {set i 0} {$i <= [expr $size + $header]} {incr i} {
    puts "$line($i)"
}

