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
            exit_error(17, name)
        self.set_data = self.module.SetDataClass(cluster)

    def add(self, node):
        self.nodes[node.name] = node
        self.set_data.add(node)
            
class ClusterClass:
    "Class definition for a cluster of nodes"
    # The names list has all the names given in the configuration file
    # in the order they wer presented.
    # See $POWERMANDIR/etc/powerman.conf for the list of nodes and types.
    # The q_types dictionary is indexed by the name of the command used to
    # query a node's state and has a list of all the nodes that are queried
    # by that command.
    # The q_com dictionary is indexed by the name of the command used to
    # query a node's state and has the command line string with which to
    # query the state of a node of that type
    # The c_types dictionary is indexed by the name of the command used to
    # set a node's state and has a list of all the nodes that are set
    # by that command.
    # The c_com dictionary is indexed by the name of the command used to
    # set a node's state and has the command line string with which to
    # set the state of a node of that type
    name     = "cluster"
    nodes    = {}
    q_types  = {}
    c_types  = {}
    fanout   = 256
    
    def __init__(self, config_file):
        "ClusterClass initialization routine"
        # The file pointed to by cfg is read line-by-line to get, first, the
        # cluster's name, and then the list of nodes in the cluster.
        # Each node is represented in the file by a line beginning with
        # the keyword node, and followed by its name and the name of the
        # command used to query its state (its query type or q_type) then
        # by the name of the command with which to set the node's state
        # (its command type or c_type).
        # Lines whose first non-whitepace character is a # are ignored.
        # Lines that parse to zero or one tokens are ignored.  If a line
        # with exactly two tokens does not start with the word cluster
        # it is ignored.  Finally, lines that do parse to at least three
        # tokens but do not start with the word node are ignored.
        # Case is ignored in the keywords cluster and node.
        self.name     = "cluster"
        self.nodes    = {}
        self.q_types  = {}
        self.c_types  = {}
        self.fanout   = 256
        try:
            cfg = open(config_file, 'r')
        except IOError :
            exit_error (7, config_file)
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
                self.c_types[c_type].add(node)
            i = i + 1
        cfg.close()

    def do_com(self, com):
        "Pass the command on to the various node types"
        if((com == 'off') or (com == 'on') or (com == 'reset')):
            for c_type in self.c_types.keys():
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

