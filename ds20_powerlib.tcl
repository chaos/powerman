proc ds20_init {} {

}

proc ds20_do_on nodes {
    ds20_do_cmd $nodes "power on"
}

proc ds20_do_off nodes {
    ds20_do_cmd $nodes "power off"
}

proc ds20_do_reset nodes {
    ds20_do_cmd $nodes "reset"
}

proc ds20_do_cmd {nodes cmd} {
    global lib_dir
    global verbose

    source /usr/lib/conman/conman.exp
    set alpha_expect [format "%s/alpha.exp" $lib_dir]
    source $alpha_expect
    log_user 0


    conman_run 256 $nodes alpha_do_rmc_cmd $verbose $cmd
}

proc ds20_check_on nodes {
#  DSR+ means "on"
  return [ditty_check_dir $nodes "DSR+"]
}

proc ds20_check_off nodes {
#  DSR- means "off"
  return [ditty_check_dir $nodes "DSR-"]
}

proc ds20_fini {} {

}

