#!/bin/bash
if [ ! -f /tmp/powermand.log.16x16 ]
then
  touch /tmp/powermand.log.16x16
fi
for x in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
do
  for y in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
  do
    ~/src/vicebox/vicebox 2$x$y &
  done
done
tail -f /tmp/powermand.log.16x16
sleep 1
for x in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
do
  if [ ! -f /tmp/powermand.log.16x16-$x ]
  then
    touch /tmp/powermand.log.16x16-$x
  fi
  ~/src/powermand/powermand -c ~/src/powermand/conf/powerman.conf.16x16-$x &
done

