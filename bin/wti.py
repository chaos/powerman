#!/usr/bin/env python
####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov>
# Copyright (C) 2000 Regents of the University of California
# See ./DISCLAIMER
####################################################################

import sys
import commands
import string
import getopt
import os
import fcntl, FCNTL
import time
import signal
import pm_utils, pm_classes

class NodeDataClass:
    "Class definition for wti controlled nodes"
    type = ""
    tty  = ""
    port = ""
    
    def __init__(self, type, vals):
        "Node class initialization"
        self.type = type
        try:
            self.tty  = vals[0]
            self.port = vals[1]
        except IndexError:
            pm_utils.exit_error(6, str(vals))
        # print self.name, self.tty, self.port

class SetDataClass:
    "Structure in which to gather all the wti info in a single place"
    cluster   = None
    nodes     = []
    password    = ''
   
    def __init__(self, cluster):
        "Start with an empty node set"
        self.cluster = cluster
        self.nodes   = []
        TIOCM_DSR    = 0x1
        TIOCMGET     = 0x5415
        
    def add(self, node):
        self.nodes.append(node)
        
    def do_com(self):
        "carry out the requested command on each tty line"
        tty_name = ""
        for node in self.nodes:
            if (node.is_marked()):
                # print "node.message = ", node.message
                if   (node.message == "on"):    com = '1'
                elif (node.message == "off"):   com = '0'
                elif (node.message == "reset"): com = 'T'
                else: pm_utils.exit_error(3, node.message)
                node.unmark()
                if ((not tty_name) or (tty_name != node.c_data.tty)):
                    if (tty_name):
                        fcntl.lockf(tty.fileno(), FCNTL.LOCK_UN)
                        tty.close()
                    tty_name = node.c_data.tty
                    try:
                        tty = open(tty_name, 'r+')
                    except IOError:
                        pm_utils.exit_error(18, tty_name)
                    try:
                        fcntl.lockf(tty.fileno(), FCNTL.LOCK_EX | FCNTL.LOCK_NB)
                    except IOError:
                        pm_utils.exit_error(19, self.name)
                tty.write(self.password+node.c_data.port+com+'\r\n')
                


    
    
