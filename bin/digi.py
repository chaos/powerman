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
    "Class definition for digi attached nodes"
    type = ""
    tty  = ""

    def __init__(self, type, vals):
        "Node class initialization"
        self.type = type
        try:
            self.tty  = vals[0]
        except IndexError:
            pm_utils.exit_error(6, str(vals))
        # print self.name, self.tty, self.box, self.port

class SetDataClass:
    "Structure in which to gather all the digi info in a single place"
    cluster   = None
    nodes     = []
    TIOCM_DSR = 0x1
    TIOCMGET  = 0x5415
    
    def __init__(self, cluster):
        "Make an empty set of nodes."
        self.cluster = cluster
        self.nodes   = []
        TIOCM_DSR    = 0x1
        TIOCMGET     = 0x5415
        
    def add(self, node):
        self.nodes.append(node)
        
    def do_com(self):
        "Carry out the requested command on each tty line."
        for node in self.nodes:
            reverse = 0
            if (node.is_marked()):
                # print "node.message = ", node.message
                if (node.message == "rquery"): reverse = 1
                node.unmark()
                tty_name = node.q_data.tty
                try:
                    tty = open(tty_name, 'r+')
                except IOError:
                    pm_utils.exit_error(18, tty_name)
                except IOError:
                    pm_utils.exit_error(19, tty_name)
                modem = '0000'
                retval = ''
                retval = fcntl.ioctl(tty.fileno(), self.TIOCMGET, modem)
                status = ord(retval[1:2])
                status = status & self.TIOCM_DSR
                # print "Checking node", node.name, "status =", status
                if((status and not reverse) or (not status and reverse)):
                    node.mark(node.name)
                tty.close()
                


    
    
