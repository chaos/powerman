#!/bin/bash
if [ ! -f /tmp/powermand.log.1 ]
then
  touch /tmp/powermand.log.1
fi
/home/auselton/src/vicebox/vicebox 11000 &
tail -f /tmp/powermand.log.1
