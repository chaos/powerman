# powerlib.tcl 
# The library in support of power control and monitoring for use with
# powerman, clustermon, poweragent, and clusterview.
# Andrew C. Uselton
# 2001-07-11

####################################################################
#
####################################################################
# There are only two functions in the API: "check" and "power".
# Each takes two parameters:  $nodes is a list of nodes as they 
# appear in the $cluster global (field 1 of each structure in the 
# list).  For "check" the second parameter can be "on" or "off".
# For "power" the second parameter can be "on", "off", or "reset".  
# "Check" returns the subset of the nodes given that match the 
# requested state (on or off).  "Power" does not return a value
# but carries out the named operation on each of the given nodes
# if it is possible.  Both operations are idempotent and don't
# care if a node is repeated in the list. 
####################################################################
#
####################################################################

proc check {node_list onoff} {
#
#   "nodes" is a list of cluster node names (i.e. just a list, no 
# structure).  "onoff" is either "on" or "off".  "check" returns 
# the elements in the list whose state (on or off) matches the
# value of the "onoff" parameter.   
#   In order to determine the state of a node one must call a 
# hardware dependent function.  Each "type" of power monitoring
# hardware will require a different interface.  Using the details
# supplied from the $cluster list this function assembles the
# needed command in the string variable $typed_check.  There may
# be more than one hardware type and more than one node of each
# hardware type.  The nodes are partitioned by type in the 
# configuration file.  This function passes the list ($check_nodes) 
# for each type to $typed_check.  Each $node_struct in the 
# list gives a list of objects with the structure {node value ...}
# giving the node name and its hardware dependent values.
#
    global check_types
    global check
    set return_val {}

    foreach type $check_types {
        set check_nodes {}
	foreach node $node_list {
	    if {[lindex $check($node) 0] == $type} {lappend check_nodes $node}
	}
	set typed_check [format "%s_check_%s" $type $onoff]
	if {[llength $check_nodes] > 0} {
            set return_val [concat $return_val [$typed_check $check_nodes]]
	}
    }
    return $return_val
}

proc power {node_list onoffreset} {
#
#   "node_list" is a list of cluster node names (i.e. just a list, no 
# structure).  "onoffreset" is either "on", "off", or "reset".  
# "power" does not return a value.  It carries out the desired 
# action on the specified list, if possible.
#   In order to change the state of a node one must call a 
# hardware dependent function.  Each "type" of power control
# hardware will require a different interface.  Using the details
# supplied from the $cluster list this function assembles the
# needed command in the string variable $typed_power.  See the 
# comments for "check" for a discussion of the structure of the
# list $do_nodes passed to $typed_power.
# 
    global control_types
    global control

    foreach type $control_types {
	set do_nodes {}
        foreach node $node_list {
	    if {[lindex $control($node) 0] == $type} {lappend do_nodes $node}
        }
	set typed_power [format "%s_power_%s" $type $onoffreset]
	if {[llength $do_nodes] > 0} {
	    $typed_power $do_nodes
	}
    }
}
####################################################################
#   Support functions for the API.  This function rearranges the 
# information in the $cluster global and filters it based on the 
# list nodes passed in.  $nodes is just a list of node names as they 
# appear in the first field of each item of the $cluster list. 
#
####################################################################

proc lunique list {
#
# remove duplicates from a list
#
    set return_list {}
    foreach element $list {
	if { [ lsearch -exact $return_list $element] < 0 } {
	    lappend return_list $element
	}
    }
    return $return_list
}


####################################################################
#
####################################################################
#   Power monitoring support functions.  If you have a new power 
# monitoring method you will want to give it a "type" name and
# write support functions for "<type>_check_on" and 
# "<type>_check_off".  In the $cluster list in "powerman.conf" a
# node with a second field set to {<type> other stuff} will
# cause the corresponding functions to be called with "other stuff"
# passed to them uninterpreted.  
#   The list of $nodes passed to these functions (and the "power"
# implementations below) have each node's "other stuff" included, so
# each item in the list has the form {node other stuff}.
#   Within the "<type>_check_<onoff>" functions any other functions 
# can be called so long as they are provided here.  Calls from one 
# hardware dependent function should not be made to those for 
# another <type>, though.
####################################################################
#
####################################################################

####################################################################
# power monitoring type:
# digi
####################################################################


proc digi_check_on node_list {
#  DSR+ means "on"
  return [digi_check_dir $node_list "DSR+"]
}

proc digi_check_off node_list {
#  DSR- means "off"
  return [digi_check_dir $node_list "DSR-"]
}

#########################
#  digi support functions
#########################

proc digi_check_dir {node_list dir} {
    global check
    global lib_dir

    set return_val {}
    foreach node $node_list {
	set digi_port [lindex $check($node) 1]
	set digi_info [exec $lib_dir/ditty_check $digi_port]
	if {[string first $dir $digi_info] >= 0} {
	    lappend return_val $node
	}
    }
    return $return_val
}

####################################################################
# power monitoring type:
# tux
####################################################################


proc tux_check_check_on node_list {
  return [tux_check_dir $node_list "on"]
}

proc tux_check_check_off node_list {
  return [tux_check_dir $node_list "off"]
}

#########################
#  tux support functions
#########################

proc tux_check_dir {node_list dir} {
    global config_file
    set config_file_base_name [file tail $config_file]
    set state_file_name [format "/tmp/%s.state" $config_file_base_name]
    
    if { ! [file exists $state_file_name] } {
	make_state_file $state_file_name
    }
    set file_fid [open $state_file_name r]
    set return_val {}
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
    }
    close $file_fid
    return $return_val
}

proc make_state_file name {
    global cluster

    set state_file [open $name w]
    foreach node_struct $cluster {
	set node [lindex $node_struct 0]
	puts $state_file "$node off"
    }
    close $state_file
}

####################################################################
#
####################################################################
#  Power control support functions.  If you have a new power 
# control method you will want to give it a "type" name and
# write support functions for "<type>_cpower_on", 
# "<type>_power_off", and "<type>_power_reset".  In the $cluster 
# list in "powerman.conf" a node with a third field set to {<type> 
# other stuff} will cause the corresponding functions to be called 
# with "other stuff" passed to them uninterpreted.  
#   The list of $nodes passed to these functions have each node's 
# "other stuff" included, so each item in the list has the form 
# {node other stuff}.
#   Within the "<type>_power_<onoffreset>" functions any other 
# functions can be called so long as they are provided here.  Calls 
# from one hardware dependent function should not be made to 
# those for another <type>, though.
####################################################################
#
####################################################################

####################################################################
# power control type:
# wti
####################################################################


proc wti_power_on node_list {
# on command is "1"
    wti_power $node_list "1"
}

proc wti_power_off node_list {
# off command is "0"
    wti_power $node_list "0"
}

proc wti_power_reset node_list {
# reset command is "T"
    wti_power $node_list "T"
}

#########################
#  wti support functions
#########################

proc wti_power {node_list command} {
    global control

    foreach node $node_list {
	set wti_dev [lindex $control($node) 1]
	set wti_passwd [lindex $control($node) 2]
	set wti_port [lindex $control($node) 3]
	set wti_fid [open $wti_dev r+]
# There should be a check here for a failed open
# alternatively, this could become an expect script
	set wti_command [format "%s%s%s\r\n" $wti_passwd $wti_port $command]
	puts $wti_fid "$wti_command"
	close $wti_fid
    }
}

####################################################################
# power control type:
# etherwake
####################################################################

proc etherwake_power_on node_list {
#only turn on nodes that are off 
    ether_wake_toggle $node_list off
}

proc etherwake_power_off node_list {
#only turn off nodes that are on
    ether_wake_toggle $node_list on
}

proc etherwake_power_reset node_list {
#turn off nodes that are on then turn on any that are off after
# a short wait
    ether_wake_toggle $node_list on
    exec sleep 3
    ether_wake_toggle $node_list off
}

##############################
#  etherwake support functions
##############################

proc ether_wake_toggle {node_list dir} {
    global lib_dir
    global control

    foreach node $node_list {
	if {[llength [check $node $dir]] > 0} {	
	    set mac_addr [lindex $control($node) 1]
	    set ether_wake_command [format "exec %s/ether-wake %s" $lib_dir $mac_addr]
	    eval $ether_wake_command
	}
    }
}

####################################################################
# power control type:
# rmc
####################################################################

proc rmc_power_on node_list {
    global tkdanger

    if {$tkdanger} {
      safe_rmc_power $node_list "on"
    } else {
      orig_rmc_power $node_list "power on"
    }
}

proc rmc_power_off node_list {
    global tkdanger

    if {$tkdanger} {
      safe_rmc_power $node_list "off"
    } else {
      orig_rmc_power $node_list "power off"
    }
}

proc rmc_power_reset node_list {
    global tkdanger

    if {$tkdanger} {
      safe_rmc_power $node_list "r"
    } else {
      orig_rmc_power $node_list "reset"
    }
}

########################
#  rmc support functions
########################

proc safe_rmc_power {node_list cmd} {
    global lib_dir
    global verbose

    exec powerman -$cmd $node_list
}

proc orig_rmc_power {node_list cmd} {
    global lib_dir
    global verbose

    source /usr/lib/conman/conman.exp
    set alpha_expect [format "%s/alpha.exp" $lib_dir]
    source $alpha_expect
    log_user 0

    conman_run 256 $node_list alpha_do_rmc_cmd $verbose $cmd
}

####################################################################
# power control type:
# tux
####################################################################


proc tux_power_power_on node_list {
    tux_power $node_list "on"
}

proc tux_power_power_off node_list {
    tux_power $node_list "off"
}

set delay_list {}

proc tux_power_power_reset node_list {
    global delay_list

    tux_power $node_list "off"
    set delay_list $node_list
    after 3000 {
      tux_power $delay_list "on"
    }
}

#########################
#  tux support functions
#########################

proc tux_power {node_list dir} {
    global cluster
    global config_file
    set config_file_base_name [file tail $config_file]
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
	foreach node $cluster {
	    set name [lindex $node 0]
	    puts $file_bak [format "%s %s" $name "off"]
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
		puts $file_fid [format "%s %s" $name $dir]
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


####################################################################
# Global initializations for the library
# 
####################################################################

foreach node_struct $cluster {
    set node [lindex $node_struct 0]
    lappend nodes $node
    set location($node) [lindex $node_struct 1]
    lappend locations $location($node)
    set control($node) [lindex $node_struct 2]
    lappend control_types [lindex $control($node) 0]
    set check($node) [lindex $node_struct 3]
    lappend check_types [lindex $check($node) 0]
    set gifs($node) [lindex $node_struct 4]
}
if {[llength $nodes] < 1} {usage "I don't see any nodes"}

set locations [lunique $locations]
set control_types [lunique $control_types]
set check_types [lunique $check_types]

