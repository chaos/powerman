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
    name = "cluster"
    names   = []
    index   = {}
    q_types = {}
    q_com   = {}
    c_types = {}
    c_com   = {}
    def __init__(self, cfg):
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
        line = cfg.readline()
        i = 0
        while (line):
            tokens = string.split(line)
            if(tokens):
                if (tokens[0][0] == '#'):
                    pass
                elif (string.lower(tokens[0]) == 'cluster'):
                    if (len(tokens) >= 2):
                        self.name = string.lower(tokens[1])
                elif (string.lower(tokens[0]) == 'node'):
                    if (len(tokens) >= 4):
                        self.names.append(tokens[1])
                        self.index[tokens[1]] = i
                        i = i + 1
                        # powermandir is a reference to a global variable
                        self.q_com[tokens[2]] = powermandir + 'bin/' + tokens[2]
                        if ( not os.path.isfile(self.q_com[tokens[2]])):
                            if(verbose):
                                sys.stderr.write("pm: Couldn't find low level routine " + self.q_com[tokens[2]] + "\n")
                            sys.exit(1)
                        try:
                            self.q_types[tokens[2]].append(tokens[1])
                        except KeyError:
                            self.q_types[tokens[2]] = []
                            self.q_types[tokens[2]].append(tokens[1])
                        self.c_com[tokens[3]] = powermandir + 'bin/' + tokens[3]
                        if ( not os.path.isfile(self.c_com[tokens[3]])):
                            if(verbose):
                                sys.stderr.write("pm: Couldn't find low level routine " + self.c_com[tokens[3]] + "\n")
                            sys.exit(1)
                        try:
                            self.c_types[tokens[3]].append(tokens[1])
                        except KeyError:
                            self.c_types[tokens[3]] = []
                            self.c_types[tokens[3]].append(tokens[1])
            line = cfg.readline()

    def node_cmp(self, x1, x2):
        "Return -1, 0, or 1 as x1 is before, the same, or after x2"
        # based on integer comparison of the self.index value, which
        # is the order in which the config file listed the nodes.
        # Icebox may return a name:temp rather than a name, so we'll
        # want to strip that bit off to do the lookup.
        try:
            name1,stuff1 = string.split(x1, ':')
        except ValueError:
            name1 = x1
        try:
            name2,stuff2 = string.split(x2, ':')
        except ValueError:
            name2 = x2
        try:
            y1 = self.index[name1]
        except KeyError:
            if(verbose):
                sys.stderr.write("pm: " + name1 + " not found\n")
            sys.exit(1)
        try:
            y2 = self.index[name2]
        except KeyError:
            if(verbose):
                sys.stderr.write("pm: " + name2 + " not found\n")
            sys.exit(1)
        return cmp(y1, y2)

    def check(self, opts, n_list):
        "return the subset of n_list that consists of nodes that are on (resp. off for -r)"
        # opts is the contents of the command line options
        #     -f, -l, -r, and -v
        # n_list is a list of node names
        # -a and -w - are processed before coming here
        output = ""
        # Issue a separate low-level call for each query type, applying it
        # to exactly the (possibly empty) subset of n_list of the given type 
        for type in self.q_types.keys():
            if(len(self.q_types[type]) > 0):
                q_str = self.q_com[type]
                # Intersect n_list with self.q_types[type] to get c_sep_list,
                # a comma separated list of the nodes from n_list that are of
                # q_type "type"
                c_sep_list = ''
                for node in n_list:
                    try:
                        self.q_types[type].index(node)
                        if(c_sep_list):
                            c_sep_list = c_sep_list + ',' + node
                        else:
                            c_sep_list = node
                    except ValueError:
                        # No harm, node just isn't of the given type 
                        pass
                if(c_sep_list):
                    # skip on past if empty list
                    command = q_str + " -w " + c_sep_list + " " + opts
                    stat, message = commands.getstatusoutput(command)
                    if (stat == 0):
                        # If you aren't carefull you could get non-node name
                        # info into output here if the underlying routine
                        # prints an error message but fails to exit(1).
                        # This will torpedo the sort later on.  
                        output = output + message
                    else:
                        if (verbose):
                            sys.stderr.write("pm: " + message + "\n")
                            sys.exit(1)
        nodes = string.split(output)
        if (nodes):
            nodes.sort(self.node_cmp)
        return nodes

    def set(self, com, opts, n_list):
        "set the power state of a node if it is not already in that state"
        # opts is the contents of the command line options -f, -l, and -v
        # com is one of on, off, or reset
        # n_list is a list of node names
        # -a and -w - are processed before coming here
        # determine a check_list of legitimate nodes for the target
        #    of the given operation by calling self.check()
        # currently "reset" will not power on an off node
        exit_code = -1
        check_list = n_list
        for type in self.c_types.keys():
            if (type == 'etherwake'):
                # Etherwake is a toggle, so you only want to apply
                # it if the node is in the appropriate state.
                if((com == 'off') or (com == 'reset')):
                    check_list = self.check("", n_list)
                elif (com == 'on'):
                    check_list = self.check("-r", n_list)
            c_str = self.c_com[type]
            # Intersect check_list with self.q_types[type] to get
            # c_sep_list, a comma separated list
            c_sep_list = ''
            for node in check_list:
                try:
                    self.c_types[type].index(node)
                    if(c_sep_list):
                        c_sep_list = c_sep_list + ',' + node
                    else:
                        c_sep_list = node
                except ValueError:
                    pass
            if(c_sep_list):
                command = c_str + " -w " + c_sep_list + " " + opts + com
                print command
                stat, message = commands.getstatusoutput(command)
                if (stat == 0):
                    # this is confirming that something
                    # good ever happened
                    if(exit_code == -1): exit_code = 0
                else:
                    if (verbose):
                        sys.stderr.write("pm: " + message + "\n")
                    exit_code = 1
        if (exit_code == -1):
            # None of the requested nodes was in a state appropriate
            # to the requested action
            exit_code = 1
            if (verbose):
                sys.stderr.write("pm: set found no matching nodes\n")
        return exit_code



def usage(msg):
    "Tha usage message"
    print "usage:", sys.argv[0], "[-a] [-c conf] [-f fan] [-l ldir] [-L] [-q] [-r] [-t] [-V] [-w node,...] [on | off | reset]"
    print "-a       = on/off/reset all nodes"
    print "-c conf  = configuration file (default: <ldir>/etc/bogus.conf)"
    print "-f fan   = fanout for parallelism (default: 256 where implemented)"
    print "-l ldir  = powerman lirary directory (default: /usr/lib/powerman)"
    print "-L       = log commands and responses to $powermandir/log/log_file (icebox only)"
    print "-q       = be quiet about any errors that may have occurred"
    print "-r       = reverse sense, i.e. check for  nodes that are off"
    print "-t       = query temperatur rather than power status"
    print "-V       = print version and exit"
    print "-w nodes = comma separated list of nodes"
    print "-w -     = read nodes from stdin, one per line"
    print "on       = turn on nodes (the default)"
    print "off      = turn off nodes"
    print "reset    = reset nodes"
    print msg
    sys.exit(0)

# Begin main routine processing.

Version = "pm: Powerman 0.1.7"

# Check for level of permision and exit for non-root users

stat, uid = commands.getstatusoutput('/usr/bin/id -u')
if (stat == 0):
    if (uid != '0'):
        usage("You must be root to run this\n")
else:
    usage("pm: Error attempting to id -u\n")

# initialize globals
powermandir = '/usr/lib/powerman/'
config_file = '/etc/powerman.conf'
work_col    = ''
verbose     = 1
reverse     = 0
temperature = 0
names       = []
com         = ''
opts        = ''
all         = 0
fanout      = 256
setting     = 0

# Look for environment variables and set globals

try:
    test = os.environ['POWERMANDIR']
    if (os.path.isdir(test)):
        powermandir = test
except KeyError:
    pass

try:
    test = os.environ['POWERMANCONF']
    if (os.path.isfile(test)):
        config_file = test
except KeyError:
    pass

try:
    test = os.environ['W_COL']
    if (os.path.isfile(test)):
        work_col = test
except KeyError:
    pass

# Parse the command line, check for sanity, and set globals

try:
    options, args = getopt.getopt(sys.argv[1:], 'ac:f:l:LqrtVw:')
except getopt.error:
    usage("Error processing options")

if(not options and not work_col):
    usage("need at least -a, -w, or a W_COL file")
for opt in options:
    op, val = opt
    if (op == '-a'):
        all = 1
    elif (op == '-c'):
        config_file = val
    elif (op == '-f'):
        fanout = val
        opts = "-f " + val + opts
    elif (op == '-l'):
        powermandir  = val
        opts = "-l " + val + opts
    elif (op == '-L'):
        opts = "-L " + opts
    elif (op == '-q'):
        verbose = 0
        opts = "-q " + opts
    elif (op == '-r'):
        opts = "-r " + opts
        reverse = 1
    elif (op == '-t'):
        temperature = 1
        opts = "-t " + opts
    elif (op == '-V'):
        print Version
        sys.exit(0)
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
    if (args[0] == 'off'):
        com = 'off'
        setting = 1
    elif (args[0] == 'on'):
        com = 'on'
        setting = 1
    elif (args[0] == 'reset'):
        com = 'reset'
        setting = 1
    else:
        if(verbose):
            sys.stderr.write("pm: Unrecognized command " + args[0] + "\n")
        sys.exit(1)

if (setting and temperature):
    if (verbose):
        sys.stderr.write("pm: temperature check only works in query mode\n")
    sys.exit(1)
        

if (powermandir):
    if (powermandir[-1] != '/'):
        powermandir = powermandir + '/'
    if(os.path.isdir(powermandir)):
        # this is probably not strictly necessary at this point since
        # pm doesn't bring in any additional python modules
        sys.path.append(powermandir)
    else:
        if(verbose):
            sys.stderr.write("pm: Couldn\'t find library directory: " + powermandir + "\n")
        sys.exit(1)
else:
    if(verbose):
        sys.stderr.write("pm: Couldn\'t find library directory: " + powermandir + "\n")
    sys.exit(1)

# Initialize operations by creating the cluster based on the description
# given in the config file.  That information includes for each node the
# name, query function, and control function.

try:
    f = open(config_file, 'r')
    theCluster = ClusterClass(f)
    f.close()
except IOError :
    if(verbose):
        sys.stderr.write("pm: Couldn\'t open configuration file: " + config_file + "\n")
    sys.exit(1)

if(all):
    names = theCluster.names

# if names list is empty look for working collective
if (not names and work_col):
    f = open(work_col, 'r')
    name = f.readline()
    while (name):
        if (name[-1:] == '\n'):
            name = name[:-1]
        names.append(name)
        name = f.readline()
    f.close()

# We'll eventually expand regex and glob arguments to -w here
# For now just check that the included names all actually occur
# in the cluster
names_list = []
for name in names:
    try:
        theCluster.names.index(name)
        names_list.append(name)
    except ValueError:
        if(verbose):
            sys.stderr.write("pm: " + name + " is not in the " + theCluster.name + " cluster\n")
        sys.exit(1)

if(names_list == []):
    if(verbose):
        sys.stderr.write("pm: No nodes found for target of operation\n")
    sys.exit(1)

# sequence through list performing action: this will amount to a call to the
# check or set method.  If the -t option produces actual temperature output
# that will be in the string with each node name returned.  

if(setting):
    sys.exit(theCluster.set(com, opts, names_list))
else:
    nodes = theCluster.check(opts, names_list)
    for node in nodes:
        print node
    sys.exit(0)
    
