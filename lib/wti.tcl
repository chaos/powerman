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
# wti
####################################################################

namespace eval wti {}

proc wti::power {command node_list} {
    set code ""
    if { $command == "on" } {
        set code "1"
    }
    if { $command == "off" } {
        set code "0"
    }
    if { $command == "reset" } {
        set code "T"
    }
    if { $code == "" } {
        puts "wti::power does not understand $command"
    } else {    
	foreach node $node_list {
	    set wti_dev [lindex $powerlib::control($node) 1]
	    set wti_passwd [lindex $powerlib::control($node) 2]
	    set wti_port [lindex $powerlib::control($node) 3]
	    set wti_fid [open $wti_dev r+]
	    # There should be a check here for a failed open
	    # alternatively, this could become an expect script
	    set wti_command [format "%s%s%s\r\n" $wti_passwd $wti_port $code]
	    puts $wti_fid "$wti_command"
	    close $wti_fid
	}
    }
}


