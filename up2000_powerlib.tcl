proc up2000_init {} {
}

proc up2000_do_on nodes {
# on command is "1"
    wti_act $nodes "1"
}

proc up2000_do_off nodes {
# off command is "0"
    wti_act $nodes "0"
}

proc up2000_do_reset nodes {
# reset command is "T"
    wti_act $nodes "T"
}

proc wti_act {nodes command} {
    global wti_passwd
    global cluster
    global wti_dev
    
    foreach node $nodes {
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
	    [lindex $node_field 0] != "wti"} {
		incr j
		set node_field [lindex $node_struct $j]
	    }
	    if {$j < $fields} {
		set wti_dev [lindex $node_field 1]
		set wti_passwd [lindex $node_field 2]
		set wti_port [lindex $node_field 3]
		set wti_fid [open $wti_dev r+]
# There should be a check here for a failed open
# alternatively, this could become an expect script
		set wti_command [format "%s%s%s\r\n" $wti_passwd $wti_port $command]
		puts $wti_fid "$wti_command"
		close $wti_fid
	    }
	}
    }
}

proc up2000_check_on nodes {
#  DSR+ means "on"
  return [ditty_check_dir $nodes "DSR+"]
}

proc up2000_check_off nodes {
#  DSR- means "off"
  return [ditty_check_dir $nodes "DSR-"]
}

proc ditty_check_dir {nodes dir} {
    global cluster
    
    set return_val {}
    foreach node $nodes {
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
	    [lindex $node_field 0] != "ditty_port"} {
		incr j
		set node_field [lindex $node_struct $j]
	    }
	    if {$j < $fields} {
		set ditty_port [lindex $node_field 1]
		set ditty_command [format "\n" $ditty_port]
		set ditty_info [exec /usr/bin/ditty /dev/ttyD$ditty_port | grep "DSR"]
		if {[string first $dir $ditty_info] >= 0} {
		    set return_val [lappend return_val $node]
		}
	    }
	}
    }
    return $return_val
}

proc up2000_fini {} {
}

