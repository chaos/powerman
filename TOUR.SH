#!/bin/sh
####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
####################################################################

# N.B.  Running the following on a live system will be disruptive to
# that system.  There is an emulator, the "virtual icebox," for a
# cluster of nodes controlled and monitored by icebox units.  The file
# src/vicebox.c implements most of the icebox's serial communications
# protocol.  If you run it on a host connected to your testing station
# via a serial cable (and spend some effort configuring the code and
# the configuration files) you may test the icebox portion of
# powerman's functionality in a non-destructive way.

echo "Read through this file and run its commands one at a time."
echo "It would probably be a 'bade thinge' (tm) to try running"
echo "these commands as root without thinking of the consequences."
echo "  -Andrew"
exit
# if you are testing code that isn't in the standard place 
# you may want to modify POWERMANCONF and POWERMANDIR to point 
# wherever you are.
# It is a omewhat lame fact that the following works even when 
# the python modules are in the /bin directory below the main
# $POWERMANDIR.  Python's search path apparently accomodates
# this organization.  
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/powerman"
cd $POWERMANDIR

# set up alternate directory for -l tests, again this assumes that 
# $powermandir is the root of the powerman source tree in which 
# you are experimenting with changes.  If that is not the case then 
# create a directory  and alternative config file and adjust the 
# commands below (or skip those tests).  
cd ..
tar cvfz ./pm.tgz powerman
mv ./pm.tgz ..
cd ..
rm -rf pm_test
tar xvfz ./pm.tgz
mv powerman pm_test
cd $POWERMANDIR

# Try running as a regular user
bin/pm
# must be root
su
# usage message
bin/pm

# in a separate window you can get logging info
tail -f /tmp/powerman.log
# query mode
bin/pm -aL
bin/pm -aL -c etc/tux.powerman.conf  
bin/pm -aL -c etc/no_tux.powerman.conf 
bin/pm -aL -f 100 
bin/pm -aL -l /home/auselton/pm_test
bin/pm -aL -l /home/auselton/pm_testy 
bin/pm -aL -r 
bin/pm -L -w dev3,dev8,dev9,dev10,dev14
bin/pm -L -w foo,bar
bin/pm -Lr -w dev3,dev8,dev9,dev10,dev14
for i in dev3 dev8 dev9 dev10 dev14
do
  echo $i
done | bin/pm -L -w - 
export W_COL="/tmp/working_collective"
for i in dev3 dev8 dev9 dev10 dev14
do
  echo $i
done > $W_COL
bin/pm -L

# control mode
bin/pm -aL  on
bin/pm -aL |wc
bin/pm -aL  off
bin/pm -aL  |wc
bin/pm -aL  on
bin/pm -aL  |wc
bin/pm -aL  reset
bin/pm -aL  |wc
bin/pm  -L -w dev3,dev8,dev9,dev10,dev14 off
bin/pm -aL  |wc
bin/pm -L -c etc/tux.powerman.conf  -w dev3,dev8,dev9,dev10,dev14 on
bin/pm -aL  |wc
bin/pm -L -c etc/no_tux.powerman.conf  -w dev3,dev8,dev9,dev10,dev14 on
bin/pm -q -c etc/no_tux.powerman.conf  -w dev3,dev8,dev9,dev10,dev14 on
bin/pm -L -l /home/auselton/pm_test  -w dev3,dev8,dev9,dev10,dev14 off
bin/pm -aL  |wc
bin/pm -L -l /home/auselton/pm_testy  -w dev3,dev8,dev9,dev10,dev14 off
for i in dev3 dev8 dev9 dev10 dev14
do
  echo $i
done | bin/pm -L -w -  on
bin/pm -aL  |wc
bin/pm  -L off
bin/pm -aL  |wc
rm -f /tmp/working_collective
exit


