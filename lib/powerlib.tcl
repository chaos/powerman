####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-0-1:  2001-07-11
# v. 0-0-2:  2001-07-31
# v. 0-0-3:  2001-08-08
#            allow "cluster" to be empty or reinitialized 
####################################################################
#
####################################################################
# There are only two functions in the API: "check" and "power".
# Each takes two parameters:  $nodes is a list of nodes.  For "check" 
# the second parameter can be "on" or "off".
# For "power" the second parameter can be "on", "off", or "reset".  
# "Check" returns the subset of the nodes given that match the 
# requested state (on or off).  "Power" does not return a value
# but carries out the named operation on each of the given nodes
# if it is possible.  Both operations are idempotent and don't
# care if a node is repeated in the list. 
####################################################################
#
####################################################################

namespace eval powerlib {
    variable cluster
    variable nodes
    variable locations
    variable check_types
    variable control_types
# These are arrays indexed by a node
    variable location
    variable control
    variable check
    variable gifs

    proc check {node_list onoff} {
	#
	#   "nodes_list" is a list of cluster node names (i.e. just a list, no 
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
	variable check_types
# These are arrays indexed by a node
	variable check

	set return_val {}
	foreach type $check_types {
	    set check_nodes {}
	    foreach node $node_list {
		if {[lindex $check($node) 0] == $type} {lappend check_nodes $node}
	    }
	    if {[llength $check_nodes] > 0} {
		set return_val [concat $return_val [[set type]::check $onoff $check_nodes]]
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
	variable control_types
# These are arrays indexed by a node
	variable control

	foreach type $control_types {
	    set do_nodes {}
	    foreach node $node_list {
		if {[lindex $control($node) 0] == $type} {lappend do_nodes $node}
	    }
	    if {[llength $do_nodes] > 0} {
		[set type]::power $onoffreset $do_nodes
	    }
	}
    }
    
    ####################################################################
    # An apparent glaring omission from the list support set of functions
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
    # Library initializations
    # 
    ####################################################################
    proc init {file} {
	variable cluster
	variable nodes
	variable locations
	variable check_types
	variable control_types
# These are arrays indexed by a node
	variable location
	variable control
	variable check
	variable gifs

	source $file
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
	
	set locations [::powerlib::lunique $locations]
	set control_types [::powerlib::lunique $control_types]
	set check_types [::powerlib::lunique $check_types]

	foreach type $check_types {
	    source $::app::lib_dir/$type.tcl
	}	
	foreach type $control_types {
	    source $::app::lib_dir/$type.tcl
	}

    }
    
    proc fini {} {
	variable cluster
	variable nodes
	variable locations
	variable check_types
	variable control_types
# These are arrays indexed by a node
	variable location
	variable control
	variable check
	variable gifs

	foreach type $check_types {
	    namespace delete $type
	}
	foreach type $control_types {
	    namespace delete $type
	}
	unset cluster
	unset nodes
	unset locations
	unset check_types
	unset control_types
# These are arrays indexed by a node
	unset location
	unset control
	unset check
	unset gifs
    }
}
# end of "namespace eval powerlib"


