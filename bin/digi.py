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

def usage(msg):
    print "usage:", sys.argv[0], "[-a] [-c conf] [-f fan] [-l ldir] [-q] [-r] [-t] [-w node,...]"
    print "-a       = on/off/reset all nodes"
    print "-c conf  = configuration file (default: <ldir>/etc/bogus.conf)"
    print "-f fan   = fanout for parallelism (default: 256 where implemented)"
    print "-l ldir  = powerman lirary directory (default: /usr/lib/powerman)"
    print "-q       = be quiet about any errors that may have occurred"
    print "-r       = reverse sense, i.e. check for  nodes that are off"
    print "-t       = temperature (not implemented)"
    print "-w nodes = comma separated list of nodes"
    print "-w -     = read nodes from stdin, one per line"
    print msg
    sys.exit(0)

def init(f):
    "Read in the node name tty address for each node from the configuration file"
    ttys = {}
    line = f.readline()
    while (line):
        tokens = string.split(line)
        line = f.readline()
	if(len(tokens) < 2):
            continue
        if (tokens[0][0] == '#'): continue
        ttys[tokens[0]] = tokens[1]
    return ttys

# initialize globals
powermandir  = '/usr/lib/powerman/'
config_file = 'etc/digi.conf'
verbose     = 1
reverse     = 0
names       = []
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
    opts, args = getopt.getopt(sys.argv[1:], 'ac:f:l:qrtw:')
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
        reverse = 1
    elif (op == '-t'):
        usage("temperature monitoring is not implemented for the digi device\n")
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

# Check for level of permision and restrict activities for non-root users

stat, uid = commands.getstatusoutput('/usr/bin/id -u')
if (stat == 0):
    if (uid != '0'):
        if(verbose):
            sys.stderr.write("digi: You must be root to run this\n")
        sys.exit(1)
else:
    if(verbose):
        sys.stderr.write("digi: Error attempting to id -u\n")
    sys.exit(1)

if (powermandir):
    if (powermandir[-1] != '/'):
        powermandir = powermandir + '/'
    if(os.path.isdir(powermandir)):
        sys.path.append(powermandir)
        config_file = powermandir + config_file
    else:
        if(verbose):
            sys.stderr.write("digi: Couldn\'t find library directory: " + powermandir + "\n")
        sys.exit(1)
else:
    if(verbose):
        sys.stderr.write("digi: Couldn\'t find library directory: " + powermandir + "\n")
    sys.exit(1)
    
try:
    f = open(config_file, 'r')
    ttys = init(f)
    f.close()
except IOError :
    if(verbose):
        sys.stderr.write("digi: Couldn\'t find configuration file: " + config_file + "\n")
    sys.exit(1)
    
# Carry out the action.  This will
# attempt to send to every valid node, even if some of those named
# are not legitimate targets, but it will only send to known
# legitimate targets
if(all):
    names = ttys.keys()


# The mask for the DSR bit: the line to which the dongle maps the
# parallel port's status line.
# TIOCM_DSR is govorned by bit 8 of the "modem" integer wich is
# bit 1 of the second byte when encoded into the ioctl's return string.
TIOCM_DSR = 0x1
TIOCMGET  = 0x5415
for name in names:
    try:
        tty = ttys[name]
    except KeyError:
        pass
    try:
        f = open(tty, 'r+')
    except IOError:
        if(verbose):
            sys.stderr.write ("digi: Unable to access digi control device on tty " + tty + "\n")
        sys.exit()
    #try:
        #fcntl.lockf(f.fileno(), FCNTL.LOCK_EX | FCNTL.LOCK_NB)
    #except IOError:
        #if(verbose):
            #sys.stderr.write ("digi: Unable to gain exclusive lock on icebox control device on tty " + tty + "\n")
        #sys.exit(1)
    modem = '0000'
    retval = ''
    retval = fcntl.ioctl(f.fileno(), TIOCMGET, modem)
    status = ord(retval[1:2])
    status = status & TIOCM_DSR
    if((status and not reverse) or (not status and reverse)):
        print name        
    #fcntl.lockf(f.fileno(), FCNTL.LOCK_UN)
    f.close()

sys.exit(0)


