#!/bin/bash
if [ ! -f /tmp/powermand.log.wti ]
then
  touch /tmp/powermand.log.wti
fi
~/src/powermand/powermand -c ~/src/powermand/conf/powerman.conf.wti &
tail -f /tmp/powermand.log.wti
