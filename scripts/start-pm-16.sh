#!/bin/bash
if [ ! -f /tmp/powermand.log.16 ]
then
  touch /tmp/powermand.log.16
fi
for i in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
do
  ~/src/vicebox/vicebox 110$i &
done
tail -f /tmp/powermand.log.16
