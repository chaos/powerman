####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-0-1:  2001-07-11
# v. 0-0-2:  2001-07-31
####################################################################
####################################################################
# power monitoring type:
# quack_check
####################################################################

namespace eval quack_check {
    # an array over nodes
    variable state
}

proc quack_check::check {dir node_list} {
    set proc "::powerlib::quack_check::check"
    if {$::app::debug == 1} {
	puts "Entering $proc"
    }

    # This will spin while quack_power is operating
    if {[info exists ::powerlib::quack_power::quack_lock]} {
	while {$::powerlib::quack_power::quack_lock} {
	    update idletasks
	}
    }
    set return_val {}
    set code ""
    if { $dir == "on" } {
	set code "1"
    }
    if { $dir == "off" } {
	set code "0"
    }
    if { $code == "" } {
	puts "quack_check::check does not understand $dir"
    } else {    
	set config_file_base_name [file tail $app::config_file]
	set state_file_name [format "/tmp/%s.state" $config_file_base_name]
	
	if { ! [file exists $state_file_name] } {
	    ::powerlib::quack_check::make_state_file $state_file_name
	}
	set file_fid [open $state_file_name r]
	while { ! [eof $file_fid]} {
	    set line [gets $file_fid]
	    scan $line "%s %s" node dir
	    if {$dir == "on"} {
		set state($node) 1
	    } else {
		set state($node) 0
	    }
	}
	close $file_fid
	foreach node $node_list {
	    if {[info exists state($node)]} {
		if {$state($node) == $code} {
		    lappend return_val $node
		}
	    }
	}
    }
    return $return_val
}

#########################
#  quack_check support functions
#########################

proc quack_check::make_state_file name {
    set state_file [open $name w]
    foreach node $powerlib::nodes {
	puts $state_file "$node off"
    }
    close $state_file
}

