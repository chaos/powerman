#!/bin/sh
####################################################################
# $Id$
# by Andrew C. Uselton <uselton2@llnl.gov> 
# Copyright (C) 2000 Regents of the University of California
# See ../DISCLAIMER
# v. 0-1-0:  2001-08-28
#            A few hints on getting started and experimenting
####################################################################

echo "Read through this file and run its commands one at a time."
echo "It would probably be a 'bade thinge' (tm) to try running"
echo "these commands as root without thinking of the consequences."
echo "  -Andrew"
exit
# modify POWERMANDIR to point wherever you are.
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/pm"
cd $POWERMANDIR

# set up alternate directory for -l tests
cd ..
tar cvfz pm.tgz pm
mv pm.tgz ..
cd ..
tar xvfz pm.tgz
mv pm poop
su
chown root poop/bin/digi
chmod 4755 poop/bin/digi
exit
cd $POWERMANDIR

# First, test using bogus device
cp etc/tux.powerman.conf etc/powerman.conf
echo "1111111101111110111111101111111110111111111101111111110111110111" >etc/bogus.conf
echo "1011111011111011111011111101011111111101011111110101111111010110" >/home/auselton/poop/etc/bogus.conf

# usage message
pm

# query mode
pm -a -v
pm -b -v
pm -b -c etc/tux.powerman.conf -v 
pm -b -c etc/no_tux.powerman.conf -v
pm -b -f 100 -v
pm -b -l /home/auselton/poop -v 
pm -b -l /home/auselton/poopy -v
pm -a -r -v
pm -w 3,8,9,10,14 -v
pm -r -w 3,8,9,10,14 -v
for i in 3 8 9 10 14 
do
  echo $i
done | pm -w - -v
export W_COL="/tmp/working_collective"
for i in 3 8 9 10 14 
do
  echo $i
done > $W_COL
pm -v

# We can create a continuous monitor to see the results of the 
# following control commands.
# in another shell enter:
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/pm"
cd $POWERMANDIR
while [ 1 ]
do
  pm -b
  sleep 1
done 
# Then back in the first shell:
# control mode
pm -a -v on
su
pm -a -v on
pm -a -v off
pm -a -v on
pm -a -v reset
pm -v -w 3,8,9,10,14 off
# Run it a second time and you get an "empty list" error
pm -v -w 3,8,9,10,14 on
pm -v -w 3,8,9,10,14 on
pm -v -w 3,8,9,10,14 off
pm -c etc/tux.powerman.conf -v -w 3,8,9,10,14 on
pm -c etc/no_tux.powerman.conf -v -w 3,8,9,10,14 on
pm -l /home/auselton/poop -v -w 3,8,9,10,14 off
pm -l /home/auselton/poopy -v -w 3,8,9,10,14 off
for i in 3 8 9 10 14 
do
  echo $i
done | pm -w - -v on
pm -v off
exit
# leave root shell and quit "pm -b" loop in other shell


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
export POWERMANDIR="/home/auselton/src/pm"
cd $POWERMANDIR

# usage message
pm

# query mode
pm -a -v
pm -b -v
pm -b -c etc/slc.powerman.conf -v 
pm -b -c etc/no_slc.powerman.conf -v
pm -b -f 100 -v
pm -b -l /home/auselton/poop -v 
pm -b -l /home/auselton/poopy -v
pm -a -r -v
pm -w slc3,slc8,slc9,slc10,slc14 -v
pm -r -w slc3,slc8,slc9,slc10,slc14 -v
for i in slc3 slc8 slc9 slc10 slc14 
do
  echo $i
done | pm -w - -v
export W_COL="/tmp/working_collective"
for i in slc3 slc4 slc11
do
  echo $i
done > $W_COL
pm -v

# in another shell enter:
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/pm"
cd $POWERMANDIR
while [ 1 ]
do
  pm -b
  sleep 1
done 
# Then back in the first shell:
# control mode
# The following are extremely disruptive to the cluster, so don't
# do any of these unless you absolutely must.
# You may want to run a clusterview session in another window.
pm -a -v on
su
# pm -a -v on
# pm -a -v off
# pm -a -v on
# pm -a -v reset
pm -v -w slc3,slc4,slc11 off
# Run it a second time and you get an "empty list" error
pm -v -w slc3,slc4,slc11 on
pm -v -w slc3,slc4,slc11 on
pm -v -w slc3,slc4,slc11 off
pm -c etc/slc.powerman.conf -v -w slc3,slc4,slc11 on
pm -c etc/no_slc.powerman.conf -v -w slc3,slc4,slc11 on
pm -l /home/auselton/poop -v -w slc3,slc4,slc11 off
pm -l /home/auselton/poopy -v -w slc3,slc4,slc11 off
for i in slc3 slc4 slc11 
do
  echo $i
done | pm -w - -v on
pm -v reset
exit


# Next set up for testing on fc cluster
cp etc/fc.digi.conf etc/digi.conf
cp etc/fc.etherwake.conf etc/etherwake.conf
cp etc/fc.powerman.conf etc/powerman.conf
cp etc/fc.digi.conf /home/auselton/poop/etc/digi.conf
cp etc/fc.etherwake.conf /home/auselton/poop/etc/etherwake.conf
cp etc/fc.powerman.conf /home/auselton/poop/etc/powerman.conf

# open a shell on useltonium
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/pm"
cd $POWERMANDIR

# usage message
pm

# query mode
pm -a -v
pm -b -v
pm -b -c etc/fc.powerman.conf -v 
pm -b -c etc/no_fc.powerman.conf -v
pm -b -f 100 -v
pm -b -l /home/auselton/poop -v 
pm -b -l /home/auselton/poopy -v
pm -a -r -v
pm -w fc3,fc8,fc9,fc10,fc14 -v
pm -r -w fc3,fc8,fc9,fc10,fc14 -v
for i in fc3 fc8 fc9 fc10 fc14 
do
  echo $i
done | pm -w - -v
export W_COL="/tmp/working_collective"
for i in fc3 fc4 fc11 
do
  echo $i
done > $W_COL
pm -v

# in another shell enter:
export POWERMANCONF="etc/powerman.conf"
export POWERMANDIR="/home/auselton/src/pm"
cd $POWERMANDIR
while [ 1 ]
do
  pm -b
  sleep 1
done 
# Then back in the first shell:
# control mode
# The following are extremely disruptive to the cluster, so don't
# do any of these unless you absolutely must.
# You may want to run a clusterview session in another window.
pm -a -v on
su
# pm -a -v on
# pm -a -v off
# pm -a -v on
# pm -a -v reset
pm -v -w fc3,fc4,fc11 off
# Run it a second time and you get an "empty list" error
pm -v -w fc3,fc4,fc11 on
pm -v -w fc3,fc4,fc11 on
pm -v -w fc3,fc4,fc11 off
pm -c etc/fc.powerman.conf -v -w fc3,fc4,fc11 on
pm -c etc/no_fc.powerman.conf -v -w fc3,fc4,fc11 on
pm -l /home/auselton/poop -v -w fc3,fc4,fc11 off
pm -l /home/auselton/poopy -v -w fc3,fc4,fc11 off
for i in fc3 fc4 fc11 
do
  echo $i
done | pm -w - -v on
pm -v reset
exit





