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
from powerman import NodeClass

class IBNodeClass(NodeClass):
    "Class definition for icebox attached nodes"
    tty  = ""
    box  = ""
    port = ""

    def __init__(self, name, vals):
        "Node class initialization"
        self.name = name
        try:
            self.tty  = vals[0]
            self.box  = vals[1]
            self.port = vals[2]
        except IndexError:
            pm_defs.exit_error(6, str(vals))
        self.requested = 0
        # print self.name, self.tty, self.box, self.port

class PortClass:
    "Class definition for an icebox port"
    name       = ""
    node       = None
    PORT_DELAY = 0.5

    def __init__(self, node):
        "Port class initialization"
        self.name      = node.port
        self.node      = node

    def do_command(self, box, com, f):
        if (self.node.requested == 0):
            return
        # This will suppress doing a command to a node twice
        # even if it's requested twice.
        self.node.requested = 0
        # print "Doing port command", com
        if ((com == 'ph') or (com == 'pl') or (com == 'rp')):
            setting = 1
        else:
            setting = 0
        target = 'c' + box.name + com + self.name
        retry_count = 0
        good_response = 0
        while (not good_response and (retry_count < 10)):
            log("say:  " + target)
            f.write(target + '\n')
            if(setting):
                time.sleep(self.PORT_DELAY)
            signal.alarm(1)
            response = f.readline()
            signal.alarm(0)
            response = response[:-1]
            if (response[-1:] == '\r'):
                response = response[:-1]
            else:
                log("Box " + box.name + " needs its firmware updated")
                signal.alarm(1)
                f.readline()
                signal.alarm(0)
            retry_count = retry_count + 1
            log("hear:  " + response)
            if(response == ''):
                continue
            if (setting):
                if (string.lower(response) == 'ok'):
                    good_response = 1
                continue
            try:
                p,val = string.split(response, ':')
            except ValueError:
                continue
            if(p != "N" + self.name):
                continue
            good_response = 1
            if(com == 'ns'):
                state = val
                if (((state == '0') and reverse) or ((state == '1') and not reverse)):
                    print self.node.name
            elif((com == 'ts') or (com == 'tsf')):
                temps = val
                print self.node.name + ":" + temps
        if (not good_response):
            pm_defs.exit_error(20, "")

class BoxClass:
    "Class definition for an icebox"
    name             = ""
    tty              = ""
    ports            = {}
    sorted_port_list = []
    num_connected    = 0
    ICEBOX_SIZE      = 10
    BOX_DELAY        = 0.1

    def __init__(self, name, tty):
        "Box class initialization"
        self.name             = name
        self.tty              = tty
        self.ports            = {}
        self.sorted_port_list = []
        self.num_connected    = 0

    def add(self, node):
        "Add a port to the list of occupied ports on the icebox"
        port = PortClass(node)
        self.ports[port.name] = port
        self.sorted_port_list.append(port.name)
        self.num_connected    = self.num_connected + 1

    def do_command(self, com, f):
        self.num_requested = 0
        for port_name in self.ports.keys():
            if (self.ports[port_name].node.requested):
                self.num_requested = self.num_requested + 1
        if (self.num_requested == self.ICEBOX_SIZE):
            # print "Doing whole box command", com
            if ((com == 'ph') or (com == 'pl') or (com == 'rp')):
                setting = 1
            else:
                setting = 0
            target = 'c' + self.name + com
            retry_count = 0
            good_response = 0
            while (not good_response and (retry_count < 10)):
                log("say:  " + target)
                f.write(target + '\n')
                if((com == 'ph') or (com == 'pl') or (com == 'rp')):
                    time.sleep(self.BOX_DELAY)
                signal.alarm(1)
                response = f.readline()
                signal.alarm(0)
                response = response[:-1]
                if (response[-1:] == '\r'):
                    response = response[:-1]
                else:
                    log("box " + str(self.name) + " needs its firmware updated")
                    signal.alarm(1)
                    f.readline()
                    signal.alarm(0)
                retry_count = retry_count + 1
                log("hear:  " + response)
                if(response == ''):
                    continue
                if (setting):
                    if (string.lower(response) == 'ok'):
                        good_response = 1
                    continue
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
                    this_port = p[1:2]
                    try:
                        nm = self.ports[this_port].node.name
                    except KeyError:
                        continue
                    good_response = 1
                    if (com == 'ns'):
                        state = val
                        if (((state == '0') and reverse) or ((state == '1') and not reverse)):
                            print nm
                    elif((com == 'ts') or (com == 'tsf')):
                        temps = val
                        print nm + ":" + temps
        else:
            # (num_requested != self.ICEBOX_SIZE):
            for port_name in self.sorted_port_list:
                port = self.ports[port_name]
                port.do_command(self, com, f)

class TtyClass:
    "Class definition for a tty with icebox(es) attached"
    name            = ""
    boxes           = {}
    sorted_box_list = []

    def __init__(self, name):
        "Tty class initialization"
        self.name            = name
        self.boxes           = {}
        self.sorted_box_list = []

    def do_command(self, com):
        # print "Doing tty command", com
        try:
            f = open(self.name, 'r+')
        except IOError:
            pm_defs.exit_error(13, self.name)
        try:
            fcntl.lockf(f.fileno(), FCNTL.LOCK_EX | FCNTL.LOCK_NB)
        except IOError:
            pm_defs.exit_error(14, self.name)
        for box_name in self.sorted_box_list:
            box = self.boxes[box_name]
            box.do_command(com, f)
        fcntl.lockf(f.fileno(), FCNTL.LOCK_UN)
        f.close()

    def add(self, node):
        try:
            box = self.boxes[node.box]
        except KeyError:
            box = BoxClass(node.box, self.name)
            self.boxes[box.name] = box
            self.sorted_box_list.append(box.name)
        box.add(node)


class IceBoxClusterClass:
    "Structure in which to gather all the icebox info in a single place"
    nodes            = {}
    ttys             = {}
    sorted_tty_list  = []

    def __init__(self, nodes):
        "Read in the node, tty, box, and port for each node from the configuration file"
        for node in nodes:
            if (node.c_type != "icebox"):
                continue
            self.add(node)

    def add(self, node):
        self.nodes[node.name] = node
        try:
            tty = self.ttys[node.tty]
        except KeyError:
            tty = TtyClass(node.tty)
            self.sorted_tty_list.append(tty.name)
            self.ttys[tty.name] = tty
        tty.add(node)

    def do_command(self, req):
        "carry out the requested command on each tty line"
        if (req == 'query'):
            com = 'ns'
        elif (req == 'off'):
            com = 'pl'
        elif (req == 'on'):
            com = 'ph'
        elif (req == 'reset'):
            com = 'rp'
        elif (req == 'temp'):
            com = 'ts'
        elif (req == 'tempf'):
            com = 'tsf'
        else:
            pm_defs.exit_error(3, req)

        signal.signal(signal.SIGALRM, ReadTimeout)

        for tty_name in self.sorted_tty_list:
            tty = self.ttys[tty_name]
            tty.do_command(com)

def usage(msg):
    print "usage:", sys.argv[0], "[-a] [-c conf] [-f fan] [-l ldir] [-L] [-q] [-r] [-t] [-w node,...] [on | off | reset]"
    print "-a       = on/off/reset all nodes"
    print "-c conf  = configuration file (default: <ldir>/etc/bogus.conf)"
    print "-f fan   = fanout for parallelism (default: 256 where implemented)"
    print "-l ldir  = powerman lirary directory (default: /usr/lib/powerman)"
    print "-L       = log commands and responses to $powermandir/log/log_file"
    print "-q       = suppress printing any errors that may have occurred"
    print "-r       = reverse sense, i.e. check for  nodes that are off"
    print "-t       = return the temperature rather than power status"
    print "-w nodes = comma separated list of nodes"
    print "-w -     = read nodes from stdin, one per line"
    print "on       = turn on nodes (the default)"
    print "off      = turn off nodes"
    print "reset    = reset nodes"
    print msg
    sys.exit(0)

def config_init(config_file):
    nodes = []
    try:
        cfg = open(config_file, 'r')
    except IOError :
        pm_defs.exit_error(7, config_file)
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
        if (last < 3): continue
        # Get rid of the trailing "}"
        tokens[last] = tokens[last][:-1]
        q_type = tokens[0]
        c_type = tokens[0]
        # powermandir is a global
        # print tokens
        # print ">" + q_type + "<"
        if (q_type != "icebox"):
            continue
        node = IBNodeClass(node_name, tokens[1:])
        node.q_type = q_type
        node.c_type = c_type
        nodes.append(node)
    cfg.close()
    return nodes

def log(string):
    if(logging):
        log_file.write ("icebox: " + string + ".\n")
        log_file.flush()

error_msg = {}
error_msg[1]  = "[1]You must be root to run this"
error_msg[2]  = "[2]Error attempting to id -u"
error_msg[3]  = "[3]Unrecognized command"
error_msg[4]  = "[4]This should be a list"
error_msg[5]  = "[5]Couldn\'t find library directory"
error_msg[6]  = "[6]Config file format error"
error_msg[7]  = "[7]Couldn\'t find configuration file"
error_msg[8]  = "[8]Couldn\'t find log file"
error_msg[9]  = "[9]Unable to access icebox control device on tty"
error_msg[10] = "[10]Unable to gain exclusive lock on icebox control device on tty"
error_msg[11] = "[11]Error returned from tty"
error_msg[12] = "[12]Not a recognized node name"
error_msg[13] = "[13]Unable to access icebox control device on tty"
error_msg[14] = "[14]Unable to gain exclusive lock on icebox control device on tty"
error_msg[15] = "[15]Error returned from icebox"
error_msg[16] = "[16]Unexpected return (not a list) from box"
error_msg[17] = "[17]Unexpected return (not a port state pair) from box"
error_msg[18] = "[18]Unexpected return (not a list) from box"
error_msg[19] = "[19]Unexpected return (not a port temp pair) from box"
error_msg[20] = "[20]Command failed after 10 unsuccessfull reads"

def exit_error(msg_num, msg_data):

    log(error_msg[msg_num] + ": " + msg_data)
    if(verbose):
        sys.stderr.write ("icebox: " + error_msg[msg_num] + ": " + msg_data + ".\n")
    sys.exit(1)

def ReadTimeout(sig, stack):
    log("Readline timed out")

verbose = 1
logging = 0
reverse = 0

def init_icebox(v_flag, l_flag, r_flag):
    verbose = v_flag
    logging = l_flag
    reverse = r_flag
    
if __name__ == '__main__':
    # initialize globals
    powermandir  = '/usr/lib/powerman/'
    config_file = '/etc/powerman.conf'
    names       = []
    req         = 'query'
    opts        = ''
    all         = 0
    fanout      = 256

    # Check for level of permision and exit for non-root users
    if (os.geteuid() != 0):
        pm_defs.exit_error(1, "")

    # Look for environment variables and set globals
    try:
        test = os.environ['POWERMANDIR']
        if (os.path.isdir(test)):
            powermandir = test
    except KeyError:
        pass
    try:
        test = os.environ['POWERMANDIR']
        if (os.path.isfile(test)):
            config_file = test
    except KeyError:
        pass

    # Parse the command line, check for sanity, and set globals
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'ac:f:l:Lqrtw:')
    except getopt.error:
        usage("Error processing options\n")
    
    if(not opts):
        usage("provide a list of nodes")
    
    for opt in opts:
        op, val = opt
        if (op == '-a'):
            all = 1
        elif (op == '-c'):
            config_file = val
        elif (op == '-f'):
            fanout = val
        elif (op == '-l'):
            powermandir  = val
        elif (op == '-L'):
            logging  = 1
        elif (op == '-q'):
            verbose = 0
        elif (op == '-r'):
            reverse = 1
        elif (op == '-t'):
            req = 'temp'
        elif (op == '-w'):
            if (val == '-'):
                name = sys.stdin.readline()
                while (name):
                    if (name[-1:] == '\n'):
                        name = name[:-1]
                    names.append(name)
                    name = sys.stdin.readline()
            else:
                names = string.split(val, ',')
        else:
            usage("Unrecognized option " + op + "\n")
    
    if (args and args[0]):
        req = args[0]
    
    if (powermandir):
        if (powermandir[-1] != '/'):
            powermandir = powermandir + '/'
        if(os.path.isdir(powermandir)):
            sys.path.append(powermandir)
        else:
            pm_defs.exit_error(5, powermandir)
    else:
        pm_defs.exit_error(5, powermandir)

    if(logging):
        try:
            log_file_name = "/tmp/icebox.log"
            log_file = open(log_file_name, 'a')
        except IOError :
            logging = 0
            exit_error(8, log_file_name)
    log("\n\n" + time.asctime(time.localtime(time.time())))

    theNodes = config_init(config_file)
    iceboxes = IceBoxClusterClass(theNodes)

    if (all):
        names = iceboxes.nodes.keys()
    for name in iceboxes.nodes.keys():
        iceboxes.nodes[name].mark()

    iceboxes.do_command(req)
    
    sys.exit(0)
    
    
