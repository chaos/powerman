#!/bin/sh
####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
####################################################################

echo "Read through this file and run its commands one at a time."
echo "It would probably be a 'bade thinge' (tm) to try running"
echo "these commands as root without thinking of the consequences."
echo "  -Andrew"
exit
# modify POWERMANDIR to point wherever you are.
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/powerman"
cd $POWERMANDIR

# set up alternate directory for -l tests
cd ..
tar cvfz ./pm.tgz powerman
mv ./pm.tgz ..
cd ..
rm -rf poop
tar xvfz ./pm.tgz
mv powerman poop
cd $POWERMANDIR

# First, test using bogus device
cp etc/tux.powerman.conf etc/powerman.conf
echo "1111111101111110111111101111111110111111111101111111110111110111" >etc/bogus.conf
echo "1011111011111011111011111101011111111101011111110101111111010110" >/home/auselton/poop/etc/bogus.conf


./pm
# must be root
su
# usage message
./pm

# query mode
./pm -a 
./pm -a -c etc/tux.powerman.conf  
./pm -a -c etc/no_tux.powerman.conf 
./pm -a -f 100 
./pm -a -l /home/auselton/poop  
./pm -a -l /home/auselton/poopy 
./pm -a -r 
./pm -w 3,8,9,10,14 
./pm -w foo,bar
./pm -r -w 3,8,9,10,14 
for i in 3 8 9 10 14 
do
  echo $i
done | ./pm -w - 
export W_COL="/tmp/working_collective"
for i in 3 8 9 10 14 
do
  echo $i
done > $W_COL
./pm 

# control mode
./pm -a  on
./pm -a  |wc
./pm -a  off
./pm -a  |wc
./pm -a  on
./pm -a  |wc
./pm -a  reset
./pm -a  |wc
./pm  -w 3,8,9,10,14 off
./pm -a  |wc
# Run it a second time and you get an "empty list" error
./pm  -w 3,8,9,10,14 on
./pm  -w 3,8,9,10,14 on
./pm  -q -w 3,8,9,10,14 on
./pm  -w 3,8,9,10,14 off
./pm -a  |wc
./pm -c etc/tux.powerman.conf  -w 3,8,9,10,14 on
./pm -a  |wc
./pm -c etc/no_tux.powerman.conf  -w 3,8,9,10,14 on
./pm -l /home/auselton/poop  -w 3,8,9,10,14 off
./pm -a  |wc
./pm -l /home/auselton/poopy  -w 3,8,9,10,14 off
for i in 3 8 9 10 14 
do
  echo $i
done | ./pm -w -  on
./pm -a  |wc
./pm  off
./pm -a  |wc
rm -f /tmp/working_collective
exit


# Next set up for testing on slc cluster
cp etc/slc.digi.conf etc/digi.conf
cp etc/slc.etherwake.conf etc/etherwake.conf
cp etc/slc.powerman.conf etc/powerman.conf
cp etc/slc.wti.conf /home/auselton/poop/etc/wti.conf
cp etc/slc.digi.conf /home/auselton/poop/etc/digi.conf
cp etc/slc.etherwake.conf /home/auselton/poop/etc/etherwake.conf
cp etc/slc.powerman.conf /home/auselton/poop/etc/powerman.conf
cp etc/slc.wti.conf /home/auselton/poop/etc/wti.conf

# open a shell on tritium
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/powerman"
cd $POWERMANDIR

# usage message
./pm
su
./pm

# query mode
# It would help if you go ahead and shut down at least one node
./pm -a 
./pm -a -r 
./pm -a -c etc/slc.powerman.conf  
./pm -a -c etc/no_slc.powerman.conf 
./pm -a -f 100 
./pm -a -l /home/auselton/poop  
./pm -a -l /home/auselton/poopy 
./pm -w slc2,slc3,slc8,slc9,slc10,slc14 
./pm -r -w slc2,slc3,slc8,slc9,slc10,slc14 
for i in slc2 slc3 slc8 slc9 slc10 slc14 
do
  echo $i
done | ./pm -w - 
export W_COL="/tmp/working_collective"
for i in slc2 slc3 slc4 slc11
do
  echo $i
done > $W_COL
./pm 

# control mode
# The following are extremely disruptive to the cluster, so don't
# do any of these unless you absolutely must.  On the SLC cluster 
# power control for slc[0-5] is via wti remote power controller, 
# for slc[6-7] is via Ethernet Wake-on-LAN, and for slc[8-15] is via
# conman.exp mediated script communicating with the RMC service 
# processor interface.  It is helpful to have a conman session 
# going on a target node to monitor its progress.
# ./pm -a  on
# ./pm -a  off
# ./pm -a  on
# ./pm -a  reset
./pm  -w slc2,slc3,slc4,slc11 off
# Run it a second time and you get an "empty list" error
./pm  -w slc2,slc3,slc4,slc11 on
./pm  -w slc2,slc3,slc4,slc11 on
./pm  -w slc2,slc3,slc4,slc11 off
./pm -c etc/slc.powerman.conf  -w slc2,slc3,slc4,slc11 on
./pm -c etc/no_slc.powerman.conf  -w slc2,slc3,slc4,slc11 on
./pm -l /home/auselton/poop  -w slc2,slc3,slc4,slc11 off
./pm -l /home/auselton/poopy  -w slc2,slc3,slc4,slc11 off
for i in slc2 slc3 slc4 slc11 
do
  echo $i
done | ./pm -w -  on
./pm  reset


# Next set up for testing on fc cluster
cp etc/fc.digi.conf etc/digi.conf
cp etc/fc.etherwake.conf etc/etherwake.conf
cp etc/fc.powerman.conf etc/powerman.conf
cp etc/fc.digi.conf /home/auselton/poop/etc/digi.conf
cp etc/fc.etherwake.conf /home/auselton/poop/etc/etherwake.conf
cp etc/fc.powerman.conf /home/auselton/poop/etc/powerman.conf

# open a shell on useltonium
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/powerman"
cd $POWERMANDIR

# usage message
./pm

# query mode
./pm -a 
./pm -a -c etc/fc.powerman.conf  
./pm -a -c etc/no_fc.powerman.conf 
./pm -a -f 100 
./pm -a -l /home/auselton/poop  
./pm -a -l /home/auselton/poopy 
./pm -a -r 
./pm -w fc3,fc8,fc9,fc10,fc14 
./pm -r -w fc3,fc8,fc9,fc10,fc14 
for i in fc3 fc8 fc9 fc10 fc14 
do
  echo $i
done | ./pm -w - 
export W_COL="/tmp/working_collective"
for i in fc3 fc4 fc11 
do
  echo $i
done > $W_COL
./pm 

# control mode
# The following are extremely disruptive to the cluster, so don't
# do any of these unless you absolutely must.
# You may want to run a clusterview session in another window.
# ./pm -a  on
# ./pm -a  off
# ./pm -a  on
# ./pm -a  reset
./pm  -w fc3,fc4,fc11 off
# Run it a second time and you get an "empty list" error
./pm  -w fc3,fc4,fc11 on
./pm  -w fc3,fc4,fc11 on
./pm  -w fc3,fc4,fc11 off
./pm -c etc/fc.powerman.conf  -w fc3,fc4,fc11 on
./pm -c etc/no_fc.powerman.conf  -w fc3,fc4,fc11 on
./pm -l /home/auselton/poop  -w fc3,fc4,fc11 off
./pm -l /home/auselton/poopy  -w fc3,fc4,fc11 off
for i in fc3 fc4 fc11 
do
  echo $i
done | ./pm -w -  on
./pm  reset
exit

# Next set up for testing on icebox test cluster
cp etc/icetest.icebox.conf etc/icebox.conf
cp etc/icetest.powerman.conf etc/powerman.conf
cp etc/icetest.icebox.conf /home/auselton/poop/etc/icebox.conf
cp etc/icetest.powerman.conf /home/auselton/poop/etc/powerman.conf

# open a shell on deuterium
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/powerman"
cd $POWERMANDIR

# usage message
./pm

# query mode
./pm -a 
./pm -a -c etc/icetest.powerman.conf  
./pm -a -c etc/no_ice.powerman.conf 
./pm -a -f 100 
./pm -a -l /home/auselton/poop  
./pm -a -l /home/auselton/poopy 
./pm -a -r 
./pm -w cyclops01,cyclops02 
./pm -r -w cyclops01,cyclops02 
for i in  cyclops01 cyclops02
do
  echo $i
done | ./pm -w - 
export W_COL="/tmp/working_collective"
for i in cyclops01 cyclops02
do
  echo $i
done > $W_COL
./pm 

# control mode
# The following are extremely disruptive to the cluster, so don't
# do any of these unless you absolutely must.
# You may want to run a clusterview session in another window.
./pm -a  on
./pm -a  off
./pm -a  on
./pm -a  reset
./pm  -w cyclops01,cyclops02 off
# Run it a second time and you get an "empty list" error
./pm  -w cyclops01,cyclops02 on
./pm  -w cyclops01,cyclops02 on
./pm  -w cyclops01,cyclops02 off
./pm -c etc/slc.powerman.conf  -w cyclops01,cyclops02 on
./pm -c etc/no_slc.powerman.conf  -w cyclops01,cyclops02on
./pm -l /home/auselton/poop  -w cyclops01,cyclops02 off
./pm -l /home/auselton/poopy  -w cyclops01,cyclops02 off
for i in cyclops01 cyclops02
do
  echo $i
done | ./pm -w -  on
./pm  reset
exit


