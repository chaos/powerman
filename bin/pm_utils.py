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
import signal

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
error_msg[22] = "[22]Faild to get termios attrs for tty"
error_msg[23] = "[23]Faild to set termios attrs for tty"
error_msg[24] = "[24]Faild to restore termios attrs for tty"

verbose = 1
logging = 0
tracing = 0
log_file = None
trace_file = None
powermandir = ""
def init_utils(v_flag, l_flag, t_flag, dir):
    global verbose
    global logging
    global tracing
    global log_file
    global trace_file
    global powermandir
    log_file_name = "/tmp/powerman.log"
    trace_file_name = "/tmp/powerman.trace"
    
    powermandir = dir
    verbose = v_flag
    logging = l_flag
    tracing = t_flag
    if(logging):
        try:
            log_file = open(log_file_name, 'a')
        except IOError :
            logging = 0
            exit_error(8, log_file_name)

    if (tracing):
        try:
            trace_file = open(trace_file_name, 'a')
        except IOError :
            tracing = 0
            exit_error(8, trace_file_name)

    log("\n\n" + time.asctime(time.localtime(time.time())))
    # end of init_utils

    
def log(string):
    global logging
    global log_file
    
    if(logging):
        log_file.write ("Powerman: " + string + ".\n")
        log_file.flush()

def trace(string):
    global tracing
    global trace_file
    
    if(tracing):
        for i in range(len(string)):
            trace_file.write (str(ord(string[i:i+1])) + " ")
        trace_file.write("\n")
        trace_file.flush()

def exit_error(msg_num, msg_data):
    global verbose

    log(error_msg[msg_num] + ": " + msg_data)
    if(verbose):
        sys.stderr.write ("Powerman: " + error_msg[msg_num] + ": " + msg_data + ".\n")
    sys.exit(msg_num)

def node_cmp(n1, n2):
    "comparison function for sorting nodes"
    return(cmp(n1.index, n2.index))

def prompt(fd, string, delay):
    log("say:  " + string)
    os.write(fd, string + '\r\n')
    if (delay): time.sleep(delay)
    done = 0
    retry_count = 0
    response = ""
    while (not done and (retry_count < 10)):
        retry_count = retry_count + 1
        signal.alarm(READ_TIMEOUT)
        try:
            response = response + os.read(fd, BUF_SIZE)
        except OSError:
            pass
        signal.alarm(ALARM_OFF)
        if (response[-1:] == '\n'):
            done = 1
    if(len(response)):
        trace(response)
        response = response[:-1]
        if (response[-1:] == '\r'):
            response = response[:-1]
        else:
            log("icebox needs its firmware updated")
            signal.alarm(READ_TIMEOUT)
            extra = ''
            try:
                extra = os.read(fd, BUF_SIZE)
            except OSError:
                pass
            signal.alarm(ALARM_OFF)
            trace(extra)
    log("hear:  " + response)
    return response

def init_alarm():
    signal.signal(signal.SIGALRM, ReadTimeout)


def ReadTimeout(sig, stack):
    log("timed out in os.read")

BUF_SIZE     = 1024
ALARM_OFF    = 0
READ_TIMEOUT = 1
