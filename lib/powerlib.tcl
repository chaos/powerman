####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-0-1:  2001-07-11
# v. 0-0-2:  2001-07-31
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

namespace eval powerlib {}

proc powerlib::check {node_list onoff} {
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
    set return_val {}
    foreach type $powerlib::check_types {
        set check_nodes {}
	foreach node $node_list {
	    if {[lindex $powerlib::check($node) 0] == $type} {lappend check_nodes $node}
	}
	if {[llength $check_nodes] > 0} {
            set return_val [concat $return_val [[set type]::check $onoff $check_nodes]]
	}
    }
    return $return_val
}

proc powerlib::power {node_list onoffreset} {
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
    foreach type $powerlib::control_types {
	set do_nodes {}
        foreach node $node_list {
	    if {[lindex $powerlib::control($node) 0] == $type} {lappend do_nodes $node}
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
proc powerlib::Init {file} {
    source $file
    foreach node_struct $cluster {
	set node [lindex $node_struct 0]
	lappend ::powerlib::nodes $node
	set ::powerlib::location($node) [lindex $node_struct 1]
	lappend ::powerlib::locations $::powerlib::location($node)
	set ::powerlib::control($node) [lindex $node_struct 2]
	lappend ::powerlib::control_types [lindex $::powerlib::control($node) 0]
	set ::powerlib::check($node) [lindex $node_struct 3]
	lappend ::powerlib::check_types [lindex $::powerlib::check($node) 0]
	set ::powerlib::gifs($node) [lindex $node_struct 4]
    }
    if {[llength $::powerlib::nodes] < 1} {usage "I don't see any nodes"}

    set ::powerlib::locations [lunique $::powerlib::locations]
    set ::powerlib::control_types [lunique $::powerlib::control_types]
    set ::powerlib::check_types [lunique $::powerlib::check_types]
}

powerlib::Init $app::config_file

foreach type $powerlib::check_types {
    source $app::lib_dir/$type.tcl
}
foreach type $powerlib::control_types {
    source $app::lib_dir/$type.tcl
}
