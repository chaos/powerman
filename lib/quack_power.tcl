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
# quack_power
####################################################################

namespace eval quack_power {}

proc quack_power::power {command node_list} {
    set code ""
    if { $command == "on" } {
        set code "1"
    }
    if { $command == "off" } {
        set code "0"
    }
    if { $command == "reset" } {
        set code "T"
	quack_power::power "off" $node_list
	if {$app::tkdanger} {
	    set ::quack_power::delay_list $node_list
	    after 3000 {
		quack_power::power "on" $::quack_power::delay_list
	    }
	} else {
	    after 3000 
	    quack_power::power "on" $node_list
	}
	return
    }
    if { $code == "" } {
        puts "etherwake::power does not understand $command"
    } else {
	set config_file_base_name [file tail $app::config_file]
	set state_file_name [format "/tmp/%s.state" $config_file_base_name]
	set backup_file_name [format "/tmp/%s.state.bak" $config_file_base_name]
	if {[file exists $state_file_name]} {
	    set file_fid [open $state_file_name r]
	    set file_bak [open $backup_file_name w]
	    while {! [eof $file_fid]} {
		set line [gets $file_fid]
		if {[string length $line] > 0} {
		    puts $file_bak $line
		}
	    }
	    close $file_fid
	    close $file_bak
	} else {
	    set file_bak [open $backup_file_name w]
	    foreach node $powerlib::nodes {
		puts $file_bak [format "%s %s" $node "off"]
	    }
	    close $file_bak
	}
	set file_fid [open $state_file_name w]
	set file_bak [open $backup_file_name r]
	while { ! [eof $file_bak]} {
	    set line [gets $file_bak]
	    set found 0
	    foreach node $node_list {
		set name [format "%s " $node]
		if {[string first $name $line] >= 0} {
		    puts $file_fid [format "%s %s" $name $command]
		    set found 1
		}
	    }
	    if {! $found } { 
		if {[string length $line] > 0} {
		    puts $file_fid $line
		}
	    }
	}
	close $file_bak
	close $file_fid
    }
}


