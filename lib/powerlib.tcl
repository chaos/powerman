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

proc check {nodes onoff} {
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
    set return_val {}

# the power monitoring type is field 2 of the $cluster structure for a node
    foreach type [type_list 2 $nodes] {
	set check_nodes [curry $type $nodes]
	set typed_check [format "%s_check_%s" $type $onoff]
	set return_val [concat $return_val [$typed_check $check_nodes]]
    }
    return $return_val
}

proc power {nodes onoffreset} {
#
#   "nodes" is a list of cluster node names (i.e. just a list, no 
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

# the power control type is field 1 of the node struct 
    foreach type [type_list 1 $nodes] {
	set do_nodes [curry $type $nodes]
	set typed_power [format "%s_power_%s" $type $onoffreset]
	$typed_power $do_nodes
    }
}

proc list_nodes {} {
    global cluster
    
    set nodes {}
    foreach node_struct $cluster {
	set nodes [lappend nodes [lindex $node_struct 0]]
    }
    return $nodes
}
####################################################################
#   Support functions for the API.  This function rearranges the 
# information in the $cluster global and filters it based on the 
# list nodes passed in.  $nodes is just a list of node names as they 
# appear in the first field of each item of the $cluster list. 
#
####################################################################

proc type_list {field nodes} {
#
# look at the list of nodes and get a list of the distinct types
# represented in the list (at position $field in the $cluster list).
#

    global cluster
    set types {}

    foreach node_struct $cluster {
	set name [lindex $node_struct 0]
	if {[lsearch -exact $nodes $name] >= 0} {
	    set types [lappend types [lindex $node_struct $field]]
	}
    }
    return [lunique $types]
}


proc lunique list {
#
# remove duplicates from a list
#
    set return_list {}

    foreach element $list {
	if { [ lsearch -exact $return_list $element] < 0 } {
	    set return_list [lappend retur_list $element]
	}
    }
    return $return_list
}

proc curry {type nodes} {
    global $type
    set return_list {}

    foreach element $[set $type] {
	if {[lsearch -exact $nodes [lindex $element 0]] >= 0} {
	    set return_list [lappend return_list $element]
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


proc digi_check_on nodes {
#  DSR+ means "on"
  return [digi_check_dir $nodes "DSR+"]
}

proc digi_check_off nodes {
#  DSR- means "off"
  return [digi_check_dir $nodes "DSR-"]
}

#########################
#  digi support functions
#########################

proc digi_check_dir {nodes dir} {

    set return_val {}
    foreach node $nodes {
	set digi_port [lindex $node 1]
	set digi_info [exec /usr/bin/ditty /dev/ttyD$digi_port | grep "DSR"]
	if {[string first $dir $digi_info] >= 0} {
	    set return_val [lappend return_val [lindex $node 0]]
	}
    }
    return $return_val
}

####################################################################
# power monitoring type:
# tux
####################################################################


proc tux_check_check_on nodes {
  return [tux_check_dir $nodes "on"]
}

proc tux_check_check_off nodes {
  return [tux_check_dir $nodes "off"]
}

#########################
#  tux support functions
#########################

proc tux_check_dir {nodes dir} {
    global config_file
    set config_file_base_name [file tail $config_file]
    set state_file_name [format "/tmp/%s.state" $config_file_base_name]
    
    if { ! [file exists $state_file_name] } {
	make_state_file $state_file_name
    }
    set return_val {}
    foreach node $nodes {
	set file_line [format "%s " [lindex $node 0]]
	set file_info [exec grep $file_line $state_file_name]
	if {[string first $dir $file_info] >= 0} {
	    set return_val [lappend return_val [lindex $node 0]]
	}
    }
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


proc wti_power_on nodes {
# on command is "1"
    wti_power $nodes "1"
}

proc wti_power_off nodes {
# off command is "0"
    wti_power $nodes "0"
}

proc wti_power_reset nodes {
# reset command is "T"
    wti_power $nodes "T"
}

#########################
#  wti support functions
#########################

proc wti_power {nodes command} {

    foreach node $nodes {
	set wti_dev [lindex $node 1]
	set wti_passwd [lindex $node 2]
	set wti_port [lindex $node 3]
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

proc etherwake_power_on nodes {
#only turn on nodes that are off 
    ether_wake_toggle $nodes off
}

proc etherwake_power_off nodes {
#only turn off nodes that are on
    ether_wake_toggle $nodes on
}

proc etherwake_power_reset nodes {
#turn off nodes that are on then turn on any that are off after
# a short wait
    ether_wake_toggle $nodes on
    exec sleep 3
    ether_wake_toggle $nodes off
}

##############################
#  etherwake support functions
##############################

proc ether_wake_toggle {nodes dir} {
    global lib_dir

    foreach node $nodes {
	if {[llength [check [lindex $node 0] $dir]] > 0} {	
	    set mac_addr [lindex $node 1]
	    set ether_wake_command [format "exec %s/ether-wake %s" $lib_dir $mac_addr]
	    eval $ether_wake_command
	}
    }
}

####################################################################
# power control type:
# rmc
####################################################################

proc rmc_power_on nodes {
    global tkdanger

    if {$tkdanger} {
      safe_rmc_power $nodes "on"
    } else {
      orig_rmc_power $nodes "power on"
    }
}

proc rmc_power_off nodes {
    global tkdanger

    if {$tkdanger} {
      safe_rmc_power $nodes "off"
    } else {
      orig_rmc_power $nodes "power off"
    }
}

proc rmc_power_reset nodes {
    global tkdanger

    if {$tkdanger} {
      safe_rmc_power $nodes "r"
    } else {
      orig_rmc_power $nodes "reset"
    }
}

########################
#  rmc support functions
########################

proc safe_rmc_power {structured_nodes cmd} {
    global lib_dir
    global verbose

    set nodes {}
    foreach node $structured_nodes {
	set nodes [lappend nodes [lindex $node 0]]
    }
    exec powerman -$cmd $nodes
}

proc orig_rmc_power {structured_nodes cmd} {
    global lib_dir
    global verbose

    set nodes {}
    foreach node $structured_nodes {
	set nodes [lappend nodes [lindex $node 0]]
    }
    source /usr/lib/conman/conman.exp
    set alpha_expect [format "%s/alpha.exp" $lib_dir]
    source $alpha_expect
    log_user 0

    conman_run 256 $nodes alpha_do_rmc_cmd $verbose $cmd
}

####################################################################
# power control type:
# tux
####################################################################


proc tux_power_power_on nodes {
    tux_power $nodes "on"
}

proc tux_power_power_off nodes {
    tux_power $nodes "off"
}

proc tux_power_power_reset nodes {
    tux_power $nodes "off"
    after 3000 {
      tux_power $nodes "on"
    }   
}

#########################
#  tux support functions
#########################

proc tux_power {nodes dir} {
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
	foreach node $nodes {
	    set name [format "%s " [lindex $node 0]]
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
