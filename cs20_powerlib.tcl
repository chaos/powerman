proc cs20_init {} {
}

proc cs20_do_on nodes {
#only turn on nodes that are off (DSR-)
    ether_wake_toggle $nodes "DSR-"
}

proc cs20_do_off nodes {
#only turn off nodes that are on (DSR+)
    ether_wake_toggle $nodes "DSR+"
}

proc cs20_do_reset nodes {
#turn off nodes that are on (DSR+) then turn on any that are off after
# a short wait
    ether_wake_toggle $nodes "DSR+"
    exec sleep 3
    ether_wake_toggle $nodes "DSR-"
}

proc ether_wake_toggle {nodes dir} {
    global cluster

    set do_nodes [intersect $nodes [ditty_check_dir $nodes $dir]]
    foreach node $do_nodes {
        set i 0
        set node_struct [lindex $cluster $i]
        set length [llength $cluster]
        while {$i < $length &&
        [lindex $node_struct 0] != $node} {
            incr i
            set node_struct [lindex $cluster $i]
	}   
	if {$i < $length} {
            set j 0
            set node_field [lindex $node_struct $j]
            set fields [llength $node_struct]
            while {$j < $fields &&
	    [lindex $node_field 0] != "mac"} {
                incr j
                set node_field [lindex $node_struct $j]
	    }   
	    if {$j < $fields} {
                set mac_addr [lindex $node_field 1]
                set ether_wake_command [format "exec %s/ether-wake %s" $lib_dir $mac_addr]
                eval $ether_wake_command
	    }   
	}   
    }   
}

proc cs20_check_on nodes {
#  DSR+ means "on"
  return [ditty_check_dir $nodes "DSR+"]
}

proc cs20_check_off nodes {
#  DSR- means "off"
  return [ditty_check_dir $nodes "DSR-"]
}

proc cs20_fini {} {
}

