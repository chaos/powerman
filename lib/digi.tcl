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
# digi
####################################################################

namespace eval digi {}

proc digi::orig_check {dir node_list} {
    set code ""
    set return_val {}
    if { $dir == "on" } {
        set code "DSR+"
    }
    if { $dir == "off" } {
        set code "DSR-"
    }
    if { $code == "" } {
        puts "digi::check does not understand $dir"
    } else {
	foreach node $node_list {
	    set digi_port [lindex $powerlib::check($node) 1]
	    set digi_info [exec $app::lib_dir/ditty_check $digi_port]
	    if {[string first $code $digi_info] >= 0} {
		lappend return_val $node
	    }
	}
    }
    return $return_val
}

proc digi::check {dir node_list} {
    set code ""
    set return_val {}
    if { $dir == "on" } {
        set code "1"
    }
    if { $dir == "off" } {
        set code "0"
    }
    if { $code == "" } {
        puts "digi::check does not understand $dir"
    } else {
	set digi_info [exec $app::lib_dir/dsr-scan]
	if {$app::verbose} {puts $digi_info}
	foreach node $node_list {
	    set digi_port [lindex $powerlib::check($node) 1]
	    if {[string index $digi_info $digi_port] == $code} {
		lappend return_val $node
	    }
	}
    }
    return $return_val
}

