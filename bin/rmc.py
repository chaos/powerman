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
    
    def __init__(self, type, vals):
        "Node class initialization"
        self.type = type

class SetDataClass:
    "Structure in which to gather all the wti info in a single place"
    cluster   = None
    nodes     = []
    rmc       = ""
    
    def __init__(self, cluster):
        "Start with an empty node set"
        self.cluster = cluster
        self.nodes   = []
        self.rmc     = pm_utils.powermandir + 'rmc'
        if (not os.path.isfile(rmc)):
            pm_utils.exit_error(20, rmc)
        
    def add(self, node):
        self.nodes.append(node)
        
    def do_com(self):
        "carry out the requested command on each rmc controlled host"
        command = self.rmc + ' ' + "-f " + self.cluster.fanout + " "
        com     = ""
        for node in self.nodes:
            if (node.is_marked()):
                # print "node.message = ", node.message
                if (not com):
                    command = command + node.name
                    com = node.message
                else:
                    command = command + "," + node.name
                node.unmark()
        command = command + com
        os.system(command)
                


    
    
