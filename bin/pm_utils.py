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
import fcntl, FCNTL
import time
import signal
import termios, TERMIOS
import random

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

tty = None
def init_tty(tty_struct):
    global tty

    try:
        tty = os.open(tty_struct.name, os.O_RDWR | os.O_SYNC, 0)
    except IOError:
        exit_error(18, tty_struct.name)
    tty_struct.device = tty
    # This code was added in suport of conman's reset function
    # which will spawn n parallel calls to powerman with a
    # single node each
    retry_count = 60
    while (not tty_struct.locked and retry_count):
        retry_count = retry_count - 1
        try:
            fcntl.lockf(tty, FCNTL.LOCK_EX | FCNTL.LOCK_NB)
            tty_struct.locked = 1
        except IOError:
            if (retry_count):
                time.sleep(random.random(0.25, 1.0))
            else:
                exit_error(19, tty_struct.name)
    try:
        tty_struct.prev_attrs = termios.tcgetattr(tty)
    except IOError:
        exit_error(22, tty_struct.name)
    tty_struct.attrs = tty_struct.prev_attrs[:]
    tty_struct.attrs[0] = TERMIOS.IGNBRK | TERMIOS.IGNPAR | TERMIOS.INPCK
    tty_struct.attrs[1] = 0
    tty_struct.attrs[2] = TERMIOS.CSIZE | TERMIOS.CREAD | TERMIOS.CLOCAL
    tty_struct.attrs[3] = 0
    try:
        termios.tcsetattr(tty, TERMIOS.TCSANOW, tty_struct.attrs)
    except IOError:
        exit_error(23, tty_struct.name)

def fini_tty(tty_struct):
    try:
        termios.tcsetattr(tty_struct.device, TERMIOS.TCSANOW, tty_struct.prev_attrs)
    except IOError:
        exit_error(24, tty_struct.name)
    fcntl.lockf(tty_struct.device, FCNTL.LOCK_UN)
    os.close(tty_struct.device)
    

def prompt(string, return_immediately):
    global tty
    
    log("say:  " + string)
    try:
        os.write(tty, string + '\r\n')
    except IOError:
        log("IOError in write")
    if (return_immediately):
        return "OK"
    done = 0
    timed_out = 0
    retry_count = 0
    response = ""
    input = ""
    while (not done and not timed_out):
        retry_count = retry_count + 1
        signal.alarm(READ_TIMEOUT)
        try:
            response = response + os.read(tty, BUF_SIZE)
            signal.alarm(ALARM_OFF)
        except OSError:
            timed_out = 1
            continue
        trace(response)
        if (response[-1:] == '\n'):
            done = 1
    if (retry_count > 1):
        log("   after " + str(retry_count) + " reads")
    if((len(response) > 0) and (response[-1:] == '\n')):
        response = response[:-1]
        if (response[-1:] == '\r'):
            response = response[:-1]
        else:
            log("icebox reply did not end in \\r\\n")
            signal.alarm(READ_TIMEOUT)
            try:
                input = os.read(tty, BUF_SIZE)
                signal.alarm(ALARM_OFF)
                trace(input)
            except OSError:
                pass
    log("hear:  " + response)
    return response

def init_alarm():
    signal.signal(signal.SIGALRM, ReadTimeout)


def ReadTimeout(sig, stack):
    log("timed out in blocking read")

ALARM_OFF    = 0
READ_TIMEOUT = 1
BUF_SIZE     = 1024
