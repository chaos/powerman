#!/bin/bash
if [ ! -f /tmp/powermand.log.ice-test ]
then
  touch /tmp/powermand.log.ice-test
fi
~/src/powermand/powermand -c ~/src/powermand/conf/powerman.conf.ice-test &
tail -f /tmp/powermand.log.ice-test
