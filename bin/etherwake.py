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
    mac  = ""
    
    def __init__(self, type, vals):
        "Node class initialization"
        self.type = type
        try:
            self.mac  = vals[0]
        except IndexError:
            pm_utils.exit_error(6, str(vals))
        # print self.type, self.mac

class SetDataClass:
    "Structure in which to gather all the wti info in a single place"
    cluster   = None
    nodes     = []
    etherwake = ""
    
    def __init__(self, cluster):
        "Start with an empty node set"
        self.cluster = cluster
        self.nodes        = []
        self.etherwake = pm_utils.powermandir + 'ether-wake'
        if (not os.path.isfile(self.etherwake)):
            pm_utils.exit_error(20, self.etherwake)
        
    def add(self, node):
        self.nodes.append(node)
        
    def do_com(self):
        "carry out the requested command on each mac address"
        mac_addr = ""
        for node in self.nodes:
            if (node.is_marked()):
                # print "node.message = ", node.message
                os.system(self.etherwake + ' ' + node.c_data.mac)
                if (node.message == "reset"): 
                    time.sleep(5)
                    os.system(self.etherwake + ' ' + node.c_data.mac)
                


    
    
