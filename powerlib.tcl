proc init {} {
    global types

    foreach type $types {
	set hw_lib [format "%s_powerlib.tcl" $type]
	source $hw_lib
	set hw_init [format "%s_init" $type]
	$hw_init
    }
}

proc do_act {nodes act} {
    global types
    global cluster

    foreach type $types {
	set hw_nodes {}
	foreach node_struct $cluster {
	    if {[lindex [lindex $node_struct 1] 1]  == $type} {
		set hw_nodes [lappend hw_nodes [lindex $node_struct 0]]
	    }  
	}
	set hw_nodes [intersect $hw_nodes $nodes]
	if {[llength $hw_nodes] > 0} {
	    set hw_do_act [format "%s_do_%s" $type $act]
	    $hw_do_act $hw_nodes
	}
    }
}


proc check_dir {nodes dir} {
    global types
    global cluster

    set return_val {}
    foreach type $types {
	set hw_nodes {}
	foreach node_struct $cluster {
	    if {[lindex [lindex $node_struct 1] 1]  == $type} {
		set hw_nodes [lappend hw_nodes [lindex $node_struct 0]]
	    }  
	}
	set hw_nodes [intersect $hw_nodes $nodes]
	if {[llength $hw_nodes] > 0} {
	    set hw_check_dir [format "%s_check_%s" $type $dir]
	    set return_val [concat $return_val [$hw_check_dir $hw_nodes]]
	}
    }
    return $return_val
}

proc fini {} {
    global types

    foreach type $types {
	set hw_fini [format "%s_fini" $type]
	$hw_fini
    }
}

proc intersect {list1 list2} {
    set return_val {}
    foreach item $list1 {
	if {[lsearch -exact $list2 $item] >= 0} {
	    set return_val [lappend return_val $item]
	}
    }
    return $return_val
}