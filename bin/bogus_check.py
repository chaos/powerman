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

def usage(msg):
    print "usage:", sys.argv[0], "[-a] [-c conf] [-f fanout] [-l ldir] [-q] [-r] [-w node,...]"
    print "-a       = check all nodes"
    print "-c conf  = configuration file (default: <ldir>/etc/bogus.conf)"
    print "-f fan   = fanout for parallelism (default: 256 where implemented)"
    print "-l ldir  = powerman lirary directory (default: /usr/lib/powerman)"
    print "-q       = be quiet about any errors that may have occurred"
    print "-r       = reverse the query, i.e. print nodes that are off"
    print "-t       = query temperatur (unimplemented)"
    print "-w nodes = comma separated list of nodes"
    print "-w -     = read nodes from stdin, one per line"
    print msg
    sys.exit(0)

# Check for level of permision and exit for non-root users

if (os.geteuid != 0):
    if(verbose):
        sys.stderr.write("bogus_set: You must be root to run this\n")
    sys.exit(1)

# initialize globals
powermandir = '/usr/lib/powerman/'
config_file = 'etc/bogus.conf'
names       = []
verbose     = 1
com         = '1'
all         = 0
fanout      = 256

# Look for environment variables and set globals

try:
    test = os.environ['POWERMANDIR']
    if (os.path.isdir(test)):
        powermandir = test
except KeyError:
    pass

# Parse the command line, check for sanity, and set globals

try:
    opts, args = getopt.getopt(sys.argv[1:], 'abc:f:l:qrtw:')
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
    elif (op == '-q'):
        verbose = 0
    elif (op == '-r'):
        com = '0'
    elif (op == '-t'):
        exit(0)
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
        
if (powermandir):
    if (powermandir[-1] != '/'):
        powermandir = powermandir + '/'
    if(os.path.isdir(powermandir)):
        sys.path.append(powermandir)
        config_file = powermandir + config_file
    else:
        if(verbose):
            sys.stderr.write("bogus_check: Couldn\'t find library directory: " + powermandir + "\n")
        sys.exit(1)
else:
    if(verbose):
        sys.stderr.write("bogus_check: Couldn\'t find library directory: " + powermandir + "\n")
    sys.exit(1)

try:
    f = open(config_file, 'r')
    states = f.readline()
    f.close()
except IOError :
    if(verbose):
        sys.stderr.write("bogus_check: Couldn\'t find configuration file: " + config_file + "\n")
    sys.exit(1)

# Carry out the action.  This will
# attempt to send to every valid node, even if some of those named
# are not legitimate targets, but it will only send to known
# legitimate targets
valueerror = 0
if (all):
    for i in range(len(states)):
        if (states[i] == com):
            print i
    sys.exit(0)
for name in names:
    try:
        pos = int(name)
    except ValueError:
        valueerror = 1
        if(verbose):
            sys.stderr.write("bogus_check: " + name + "is not an integer\n")
    try:
        if (states[pos] == com):
            print name
    except IndexError:
        pass
if(valueerror == 1):
    sys.exit(1)
sys.exit(0)


