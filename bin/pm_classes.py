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
import pm_utils

class NodeClass:
    "Class definition for a node"
    name     = ""
    q_data   = None
    c_data   = None
    state    = 0
    index    = 0
    message    = ""
    
    def __init__(self, name, q_data, c_data, index):
        self.name     = name
        self.q_data   = q_data
        self.c_data   = c_data
        self.state    = 0
        self.index    = index
        self.message  = ""

    def mark(self, message):
        self.message = message

    def unmark(self):
        self.message = ""

    def is_marked(self):
        return len(self.message)

class NodeSetClass:
    "Class definition for info about a particular collection of nodes"
    name     = ""
    nodes    = {}
    module   = None
    set_data = None

    def __init__(self, name, cluster):
        "NodeSetClass initialization routine"
        self.name = name
        self.nodes = {}
        try:
            self.module = __import__(name)
        except ImportError:
            pm_utils.exit_error(17, name)
        self.set_data = self.module.SetDataClass(cluster)

    def add(self, node):
        self.nodes[node.name] = node
        self.set_data.add(node)
            
class ClusterClass:
    "Class definition for a cluster of nodes"
    name     = "cluster"
    nodes    = {}
    q_types  = {}
    c_types  = {}
    fanout   = 256
    
    def __init__(self, config_file):
        "ClusterClass initialization routine"
        self.name     = "cluster"
        self.nodes    = {}
        self.q_types  = {}
        self.c_types  = {}
        self.fanout   = 256
        try:
            cfg = open(config_file, 'r')
        except IOError :
            pm_utils.exit_error (7, config_file)
        line = cfg.readline()
        i = 0
        while (line):
            blocks = string.split(line, '{')
            line = cfg.readline()
            if(len(blocks) < 2): continue
            if (blocks[0][0] == '#'): continue
            node_name = blocks[0]
            while (node_name[-1:] in string.whitespace): node_name = node_name[:-1]
            tokens = string.split(blocks[1])
            last = len(tokens) - 1
            if (last < 0): continue
            # Get rid of the trailing "}"
            tokens[last] = tokens[last][:-1]
            q_type = tokens[0]
            try:
                module = self.q_types[q_type].module
            except KeyError:
                self.q_types[q_type] = NodeSetClass(q_type, self)
                module = self.q_types[q_type].module
            q_data = module.NodeDataClass(q_type, tokens[1:])
            if (len(blocks) == 3):
                tokens = string.split(blocks[2])
                last = len(tokens) - 1
                if (last < 0): continue
                tokens[last] = tokens[last][:-1]
                c_type = tokens[0]
                try:
                    module = self.c_types[c_type].module
                except KeyError:
                    self.c_types[c_type] = NodeSetClass(c_type, self)
                    module = self.c_types[c_type].module
                c_data = module.NodeDataClass(c_type, tokens[1:])
            else:
                c_type = q_type
                c_data = q_data
                try:
                    c_set = self.c_types[c_type]
                except KeyError:
                    self.c_types[c_type] = self.q_types[q_type]
            node = NodeClass(node_name, q_data, c_data, i)
            self.nodes[node_name] = node
            self.q_types[q_type].set_data.add(node)
            if(len(blocks) == 3):
                self.c_types[c_type].set_data.add(node)
            i = i + 1
        cfg.close()

    def do_com(self, com):
        "Pass the command on to the various node types"
        if((com == 'off') or (com == 'on') or (com == 'reset')):
            for c_type in self.c_types.keys():
                if (c_type == "etherwake"):
                    # Deal with toggle problem here
                    if (com == "off"):
                        recursive_query = 'pm -ar' + opstr
                    else:
                        recursive_query = 'pm -a' + opstr
                    status,output = commands.getstatusoutput(recursive_query)
                    if (status):
                        # Something wrong in recursive call
                        pm_utils.exit_error(25, output)
                    ignore_nodes = string.split(output)
                    for node in c_set.nodes:
                        if (node.is_marked() and (node.name in ignore_nodes)):
                            node.unmark()
                c_set = self.c_types[c_type]
                c_set.set_data.do_com()
        else:
            for q_type in self.q_types.keys():
                q_set = self.q_types[q_type]
                q_set.set_data.do_com()

    def mark_all (self, message):
        "For subsidiary types that have a \"mark_all\" method, use it."
        for name in self.nodes.keys():
            self.nodes[name].mark(message)

