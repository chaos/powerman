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
# tux_check
####################################################################

namespace eval tux_check {}

proc tux_check::check {dir node_list} {
    set return_val {}
    set code ""
    if { $dir == "on" } {
        set code "1"
    }
    if { $dir == "off" } {
        set code "0"
    }
    if { $code == "" } {
        puts "tux_check::check does not understand $dir"
    } else {    
	set config_file_base_name [file tail $app::config_file]
	set state_file_name [format "/tmp/%s.state" $config_file_base_name]
    
	if { ! [file exists $state_file_name] } {
	    tux_check::make_state_file $state_file_name
	}
	set file_fid [open $state_file_name r]
	while { ! [eof $file_fid]} {
	    set line [gets $file_fid]
	    set mark [string first " " $line]
	    incr mark -1
	    if {$mark > 0} {
		set node [string range $line 0 $mark]
		foreach n $node_list {
		    if {$node == $n} {
			if {[string first $dir $line] >= 0} {
			    lappend return_val $node
			}
		    }
		}
	    }
	}                    ;# while
	close $file_fid
    }
    return $return_val
}

#########################
#  tux_check support functions
#########################

proc tux_check::make_state_file name {
    set state_file [open $name w]
    foreach node $powerlib::nodes {
	puts $state_file "$node off"
    }
    close $state_file
}

