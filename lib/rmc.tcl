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
# rmc
####################################################################

namespace eval rmc {}

proc rmc::power {command node_list} {
    set code ""
    if { $command == "on" } {
	if {$app::tkdanger} {
	    set code "on"
	} else {
	    set code "power on"
	}
    }
    if { $command == "off" } {
	if {$app::tkdanger} {
	    set code "off"
	} else {
	    set code "power off"
	}
    }
    if { $command == "reset" } {
	if {$app::tkdanger} {
	    set code "r"
	} else {
	    set code "reset"
	}
    }
    if { $code == "" } {
        puts "rmc::power does not understand $command"
    } else {
	if {$app::tkdanger} {
	    rmc::safe_power $code $node_list
	} else {
	    rmc::orig_power $code $node_list
	}
    }
}

########################
#  rmc support functions
########################

proc rmc::safe_power {command node_list} {
# This invokes "powerman" which will enter this library again and 
# use the "orig_power" method.  "Orig_power" adds a "-j" so the
# console can be open during the action.  
# N.B. This assumes "powerman" is in the path.  I usually put it in
# /usr/bin, but a person checking it out of cvs to try it out might
# not have completely installed it, and "." might not be in their 
# path.
    exec powerman -$command $node_list
}

proc rmc::orig_power {command node_list} {
    source /usr/lib/conman/conman.exp
    set alpha_expect [format "%s/alpha.exp" $app::lib_dir]
    source $alpha_expect
    log_user 0

    set nodes_and_options [linsert $node_list 0 "-j"]
#    conman_run 256 $node_list alpha_do_rmc_cmd $app::verbose $cmd
    conman_run 256 $nodes_and_options alpha_do_rmc_cmd $app::verbose $command
}

