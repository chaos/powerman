#!/usr/bin/env python
####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ./DISCLAIMER
####################################################################

import sys
import regex
import commands
import string
import getopt
import os
import time

error_msg = {}
error_msg[1]  = "[1]You must be root to run this"
error_msg[2]  = "[2]Error attempting to id -u"
error_msg[3]  = "[3]Unrecognized command"
error_msg[4]  = "[4]This should be a list"
error_msg[5]  = "[5]Couldn\'t find library directory"
error_msg[6]  = "[6]Config file format error"
error_msg[7]  = "[7]Couldn\'t find configuration file"
error_msg[8]  = "[8]Couldn\'t find log file"
error_msg[9]  = "[9]Not a recognized node name"
error_msg[10] = "[10]Couldn't find low level routine"
error_msg[11] = "[11]Unrecognized response"
error_msg[12] = "[12]No matching nodes"
error_msg[13] = "[13]temperature check only works in query mode"
error_msg[14] = "[14]No nodes found for target of operation"
error_msg[15] = "[15]Unrecognized query type"
error_msg[16] = "[16]Unrecognized control type"
error_msg[17] = "[17]Unrecognized module name"
error_msg[18] = "[18]Failed to open tty"
error_msg[19] = "[19]Failed to lock tty"
error_msg[20] = "[20]Failed to find helper function"
error_msg[21] = "[21]Timed out after ten attempts to access port"

verbose = 1
logging = 0
log_file = None
powermandir = ""
def init_utils(v_flag, l_flag, dir):
    global verbose
    global logging
    global log_file
    global powermandir
    log_file_name = "/tmp/powerman.log"
    
    powermandir = dir
    verbose = v_flag
    logging = l_flag
    if(logging):
        try:
            log_file = open(log_file_name, 'a')
        except IOError :
            logging = 0
            exit_error(8, log_file_name)
    log("\n\n" + time.asctime(time.localtime(time.time())))

def log(string):
    global logging
    
    if(logging):
        log_file.write ("Powerman: " + string + ".\n")
        log_file.flush()

def exit_error(msg_num, msg_data):
    global verbose

    log(error_msg[msg_num] + ": " + msg_data)
    if(verbose):
        sys.stderr.write ("Powerman: " + error_msg[msg_num] + ": " + msg_data + ".\n")
    sys.exit(msg_num)

def node_cmp(n1, n2):
    "comparison function for sorting nodes"
    return(cmp(n1.index, n2.index))

