#
# getopt.tcl
# 
# Option parsing library for Tcl scripts
# Copyright (C) SoftWeyr, 1997
# Author V. Wagner <vitus@agropc.msk.su
#
# Distributed under GNU public license. (i.e. compiling into standalone
# executables or encrypting is prohibited, unless source is provided to users)
#

#  
# getopt - recieves an array of possible options with default values
# and list of options with values, and modifies array according to supplied
# values
# ARGUMENTS: arrname - array in calling procedure, whose indices is names of
# options WITHOUT leading dash and values are default values.
# if element named "default" exists in array, all unrecognized options
# would concatenated there in same form, as they was in args
# args - argument list - can be passed either as one list argument or 
# sepatate arguments 
# RETURN VALUE: none
# SIDE EFFECTS: modifies passed array 
#
proc getopt {arrname args} {
upvar $arrname opt
if ![array exist opt] {
  return -code error "Array $arrname doesn't exist"
}
if {[llength $args]==1} {eval set args $args}
if {![llength $args]} return
if {[llength $args]%2!=0} {error "Odd count of opt. arguments"}
foreach {option value} $args {
   if [string match -* $option] {
   set list [array names opt [string trimleft $option -]*]
   } else { set list {}}
   switch -exact [llength $list] {
       0 { if [info exists opt(default)] {
	      lappend opt(default) $option $value
	   } else {
	       set msg "unknown option $option. Should be one of:"
	       foreach j [array names opt] {append msg " -" $j}
	       return -code error $msg
	   }
       }
       1 { set opt($list) $value 
       }
       default { 
         if [set j [lsearch -exact $list [string trimleft $option -]]]!=-1 {
	     set opt([lindex $list $j]) $value
	 } else { 
	     set msg "Ambiguous option $option:"
	     foreach j $list {append msg " -" $j}
	     return -code error $msg
	 }
       }
   }
}
   return
}

#  
# handleopt - recieves an array of possible options and executes given scritp
# for each of valid option passed , appending opion value to it
# ARGUMENTS: arrname - array in calling procedure, whose indices is names of
# options WITHOUT leading dash and values are corresponding scripts 
# args - argument list - can be passed either as one list argument or 
# sepatate arguments 
# if element "default" appears in array, script contained there would
# be executed for each unrecognized option with option itself and then
# its value appended
# RETURN VALUE: return value of last script executed
# SIDE EFFECTS: execiting of one or more passed scripts 
# NOTES: if you want simply return value of option, return is good candidate for
#        script. {return -return} would terminate caller 
#
proc handleopt {arrname args} {
upvar $arrname opt
if ![array exist opt] {
  return -code error "Array $arrname doesn't exist"
}
if {[llength $args]==1} {eval set args $args}
if {![llength $args]} return
if {[llength $args]%2!=0} {error "Odd count of opt. arguments"}
set result {}
foreach {option value} $args {
    if [string match -* $option] {
       set list [array names opt [string trimleft $option -]*]
    } else {set list {}}
    switch -exact [llength $list] {
       0 { if [info exist opt(default)] {
	     set cmd [linsert $opt(default) end $option $value]
	   } else {
	      set msg "unknown option $option. Should be one of:"
	     foreach j [array names opt] {append msg " -" $j}
	     return -code error $msg
	   }
       }
       1 { set cmd [linsert $opt($list) end $value]}
       default { 
	    if [set j [lsearch -exact $list [string trimleft $option -]]]!=-1 {
		set cmd [linsert $opt([lindex $list $j]) end $value]
	    } else { 
	       set msg "Ambiguous option $option:"
	       foreach j $list {append msg " -" $j}
	       return -code error $msg
	    }
       }
    }
    if [catch {uplevel $cmd} result ] {
	return -code error $result
    }
}
return $result
} 
# 
# checks variable for valid boolean value
# and replaces it with 1, if true or 0 it false. If value is not
# correct, message is stored in msg and 1 returned. Otherwise 0 is returned
#

proc checkbooleanopt {var msg} {
upvar $var test
set t [string tolower $test]
set r 0
if [string length $t] {
    foreach truth {1 yes on true} {
      if [string match $t* $truth] {
	set r 1
	break
      }
    }
    foreach lie {0 no off false} {
      if [string match $t* $lie] {
	 if $r {
	   uplevel set $msg [list "Ambiquous boolean value \"$test\""]
	   return 1
	 } else {
	   set test 0
	   return 0
	 }
      }
   }    
}
if $r {
  set test 1
  return 0
} else {
  uplevel set $msg [list "Expected boolean value, but got \"$test\""]
  return 1
}      
}
#
# checks variable value for matching one (and only one) of given list element
# and replaces its value with literal value of this element
# Returns 0 if value is correct, 1 if it is bad. Sets msg to verbose error message
#

proc checklistopt {var list msg} {
upvar $var test
upvar $msg err
foreach i $list { 
   set tmp($i) 1
}
# Ok, there is literal match
if [info exists tmp($test)] {return 0}
# Trying to do glob match
set num [llength [set found [array names tmp $test*]]]
if {$num==1} {
  set test [lindex $found 0]
  return 0
} elseif {!$num} {
  set err "Unknown option $test. Should be one of [join $list ", "]"
  return 1
} else {
  set err "Ambiquous option $test.\n [join $found ", "]"
  return 1
}
}
# Checks variable value for being integer and (optionally) fall into given range
# You can use empty string, if you don't want to check for min or max
# 
# Returns 0 if no error, 1 if something wrong. Sets msg to verbose error message
#
proc checkintopt {var min max msg} {
upvar $var test
upvar $msg err
if ![string length $min] {set min -0x7fffffff}
if ![string length $max] {set max 0x7fffffff}
set test [string trim $test]
if ![regexp {^[+-]?(0[xX][0-9A-Fa-f]+|[1-9][0-9]*|0[0-7]*)$} $test] {
   set err "Expected integer, but got \"$test\""
   return 1
}
if {$test<$min} {
  set err  "Expected integer greater than $min, but got $test"
  return 1
}
if {$test>$max} {
  set err "Expected integer less than $max, but got $test"
}
return 0
}
package provide getopt 1.1
