#!/bin/bash
if [ ! -f /tmp/powermand.log.baytech ]
then
  touch /tmp/powermand.log.baytech
fi
~/src/powermand/powermand -c ~/src/powermand/conf/powerman.conf.baytech &
tail -f /tmp/powermand.log.baytech
