####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-0-1:  2001-07-11
# v. 0-0-2:  2001-07-31
####################################################################

####################################################################
# power control type:
# etherwake
####################################################################

namespace eval etherwake {}

proc etherwake::power {command node_list} {
    set dir ""
    if { $command == "on" } {
        set dir "off"
    }
    if { $command == "off" } {
        set dir "on"
    }
    if { $command == "reset" } {
        set dir "both"
	etherwake::power "off" $node_list
	if {$tkdanger} {
	    set ::etherwake::delay_list $node_list
	    after 3000 {
		etherwake::power "on" $::etherwake::delay_list
	    }
	} else {
	    after 3000 
	    etherwake::power "on" $node_list
	}
	return
    }
    if { $dir == "" } {
        puts "etherwake::power does not understand $command"
    } else {
	foreach node $node_list {
	    if {[llength [powerlib::check $node $dir]] > 0} {	
		set mac_addr [lindex $powerlib::control($node) 1]
		set etherwake_command [format "exec %s/ether-wake %s" $app::lib_dir $mac_addr]
		eval $etherwake_command
	    }
	}
    }
}

