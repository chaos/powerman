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
import termios, TERMIOS
import random

class NodeDataClass:
    "Class definition for icebox attached nodes"
    type = ""
    tty  = ""
    box  = ""
    port = ""
    cpu1 = 0
    cpu2 = 0

    def __init__(self, type, vals):
        "Node class initialization"
        self.type = type
        self.cpu1 = 0
        self.cpu2 = 0
        try:
            self.tty  = vals[0]
            self.box  = vals[1]
            self.port = vals[2]
        except IndexError:
            pm_utils.exit_error(6, str(vals))
        # print self.name, self.tty, self.box, self.port

class PortClass:
    "Class definition for an icebox port"
    name       = ""
    node       = None
    box        = None
    com        = ""
    reply      = ""
    PORT_DELAY = 0.3
    
    def __init__(self, node, box):
        "Port class initialization"
        self.box = box
        try:
            port_name = node.q_data.port
        except AttributeError:
            port_name = node.c_data.port
        self.name      = port_name
        self.node      = node
        self.com       = ""
        
    def do_command(self):
        # print "Doing port command", com
        box = self.node.c_data.box
        target = 'c' + box + self.com + self.name
        retry_count = 0
        good_response = 0
        while (not good_response and (retry_count < 10)):
            retry_count = retry_count + 1
            response = pm_utils.prompt(target, 0)
            if (string.lower(response) == 'ok'):
                good_response = 1
        if (not good_response):
            pm_utils.exit_error(21, self.name + ",box " + box)
        time.sleep(self.PORT_DELAY)
            
class BoxClass:
    "Class definition for an icebox"
    name             = ""
    ports            = {}
    tty              = None
    com              = ""
    ICEBOX_SIZE      = 10
    # Setting BOX_DELAY to 3.5 seconds results in always having exactly
    # one "ERROR" reply before success.
    LONG_BOX_DELAY   = 4.0
    SHORT_BOX_DELAY  = 0.05
    
    def __init__(self, name, tty):
        "Box class initialization"
        self.name  = name
        self.tty   = tty
        self.ports = {}
        self.com   = ""

    def add(self, node):
        "Add a port to the list of occupied ports on the icebox"
        port = PortClass(node, self)
        self.ports[port.name] = port

    def do_command(self):
        num_requested = 0
        req = ""
        for port_name in self.ports.keys():
            port = self.ports[port_name]
            port.reply = ""
            if (port.node.is_marked()):
                if (not req): req = port.node.message
                num_requested = num_requested + 1
        if (not req): return
        setting = 0
        reverse = 0
        return_immediately = 0
        if (req == 'query'):
            com = 'ns'
        elif (req == 'rquery'):
            reverse = 1
            com = 'ns'
        elif (req == 'off'):
            setting = 1
            com = 'pl'
        elif (req == 'on'):
            setting = 1
            com = 'ph'
        elif (req == 'reset'):
            setting = 1
            com = 'rp'
        elif (req == 'temp'):
            com = 'ts'
        elif (req == 'tempf'):
            com = 'tsf'
        elif (req == 'hwreset'):
            return_immediately = 1
            com = 'rb'
        else:
            pm_utils.exit_error(3, req)
        if ((num_requested < self.ICEBOX_SIZE) and setting):
            for port_name in self.ports.keys():
                port = self.ports[port_name]
                if (port.node.is_marked()):
                    port.com = com
                    port.node.unmark()
                    port.do_command()
            return
        # print "Doing whole box command", com
        target = 'c' + self.name + com
        retry_count = 0
        good_response = 0
        while (not (good_response == num_requested) and (retry_count < 10)):
            retry_count = retry_count + 1
            response = pm_utils.prompt(target, return_immediately)
            if(response == ''):
                continue
            if (setting or return_immediately):
                if (string.lower(response) == 'ok'):
                    good_response = num_requested
                    for port_name in self.ports.keys():
                        port = self.ports[port_name]
                        port.com = ""
                        port.node.unmark()
                    self.com = ""
                    continue
            # now we're dealing with some sort of query
            try:
                list = string.split(response)
            except ValueError:
                continue
            for resp in list:
                try:
                    p,val = string.split(resp, ':')
                except ValueError:
                    continue
                if(p[0:1] != 'N'):
                    continue
                port_name = p[1:2]
                try:
                    port = self.ports[port_name]
                except KeyError:
                    continue
                node = port.node
                if(node.is_marked()):
                    if (com == 'ns'):
                        if ((val == '0') or (val == '1')):
                            node.unmark()
                            node.state = val
                            good_response = good_response + 1
                        if (((val == '0') and reverse) or ((val == '1') and not reverse)):
                            port.reply = node.name
                    elif((com == 'ts') or (com == 'tsf')):
                        node.unmark()
                        temps = val
                        try:
                            temp1, temp2 = string.split(temps, ',')
                        except ValueError:
                            continue
                        try:
                            node.q_data.cpu1 = cpu1 = int(temp1)
                            node.q_data.cpu2 = cpu2 = int(temp2)
                            good_response = good_response + 1
                        except ValueError:
                            continue
                        if((self.tty.set.low != None) and (self.tty.set.high != None)):
                            # print "Checking bounds", node.name
                            low = self.tty.set.low
                            high = self.tty.set.high
                            if((cpu1 < low) or (cpu2 < low) or (cpu1 > high) or (cpu2 > high)):
                                port.reply = "%s:%s" % (node.name,temps)
                        else:
                            port.reply = "%s:%s" % (node.name,temps)
        if (good_response == num_requested):
            for port_name in self.ports.keys():
                port = self.ports[port_name]
                port.node.mark(port.reply)
        else:
            pm_utils.exit_error(21, "box " + self.name)
        if (setting):
            time.sleep(self.LONG_BOX_DELAY)
        else:
            time.sleep(self.SHORT_BOX_DELAY)

class TtyClass:
    "Class definition for a tty with icebox(es) attached"
    name       = ""
    boxes      = {}
    set        = None
    prev_attrs = None
    attrs      = None
    device     = None
    locked     = 0

    def __init__(self, name, set):
        "Tty class initialization"
        self.name       = name
        self.boxes      = {}
        self.set        = set
        self.attrs      = None
        self.prev_attrs = None
        self.device     = None
        self.locked     = 0

    def do_command(self):
        # print "Doing tty command", com
        #old_time = time.time()
        #new_time = None
        pm_utils.init_tty(self)
        for box_name in self.boxes.keys():
            box = self.boxes[box_name]
            box.do_command()
            #new_time = time.time()
            #sys.stderr.write("    %1.3f" % (new_time - old_time))
            #old_time = new_time
        #sys.stderr.write("\n")
        pm_utils.fini_tty(self)
        
    def add(self, node):
        try:
            box_name = node.q_data.box
        except AttributeError:
            box_name = node.c_data.box
        try:
            box = self.boxes[box_name]
        except KeyError:
            box = BoxClass(box_name, self)
            self.boxes[box_name] = box
        box.add(node)


class SetDataClass:
    "Structure in which to gather all the icebox info in a single place"
    ttys    = {}
    cluster = None
    low     = None
    high    = None
    
    def __init__(self, cluster):
        "Read in the node, tty, box, and port for each node from the configuration file"
        self.ttys    = {}
        self.cluster = cluster
        
    def add(self, node):
        try:
            tty = self.ttys[node.q_data.tty]
        except KeyError:
            tty = TtyClass(node.q_data.tty, self)
            self.ttys[tty.name] = tty
        tty.add(node)

    def do_com(self):
        "carry out the requested command on each tty line"
        pm_utils.init_alarm()
        for tty_name in self.ttys.keys():
            tty = self.ttys[tty_name]
            tty.do_command()

    def set_temp_bounds(self, low, high):
        "Set the bounds for temperature queries"
        self.low  = low
        self.high = high
        # print "Using bounds:", low, high
