#!/bin/bash

VICE_SCRIPT="/tmp/powerman/vicebox.sh"
PM_SCRIPT="/tmp/powerman/powerman.sh"
PM_CONFIG="/tmp/powerman/powerman.conf"

VICE_PATH="/home/behlendo/src/powerman/test"
PM_PATH="/home/behlendo/src/powerman/src"

if [ $# -ne 1 ]; then
  echo "Usage: $0 <number of viceboxs>"
  exit 1
fi

MAX_SIZE=$1

echo "#!/bin/bash"                           >$VICE_SCRIPT
echo "#!/bin/bash"                           >$PM_SCRIPT

echo "include \"/etc/powerman/vicebox.dev\"" >$PM_CONFIG
echo ""                                     >>$PM_CONFIG
echo "begin global"                         >>$PM_CONFIG
echo "  client listener port \"10101\""     >>$PM_CONFIG
echo "  timeout interval \"0.1\""           >>$PM_CONFIG
echo "  inter-device delay \"0.5\""         >>$PM_CONFIG
echo "  update interval \"100.0\""          >>$PM_CONFIG
echo "end global"                           >>$PM_CONFIG
echo ""                                     >>$PM_CONFIG

CUR_SIZE=0
while [ $CUR_SIZE -lt $MAX_SIZE ]; do
  echo "$VICE_PATH/vicebox $[$CUR_SIZE+2000] &" >>$VICE_SCRIPT
  echo "device \"ice-$CUR_SIZE\" \"vicebox\" \"localhost\" " \
       "\"$[$CUR_SIZE+2000]\""                  >>$PM_CONFIG
  CUR_SIZE=$[$CUR_SIZE+1]
done

echo ""            >>$PM_CONFIG
echo "begin nodes" >>$PM_CONFIG

CUR_SIZE1=0
CUR_SIZE2=0
while [ $CUR_SIZE1 -lt $MAX_SIZE ]; do
  CUR_SIZE3=1

  while [ $CUR_SIZE3 -lt 11 ]; do
    echo "node \"tux$CUR_SIZE2\" \"ice-$CUR_SIZE1\" \"$CUR_SIZE3\"">>$PM_CONFIG
    CUR_SIZE2=$[CUR_SIZE2+1]
    CUR_SIZE3=$[CUR_SIZE3+1]
  done

  echo ""         >>$PM_CONFIG
  CUR_SIZE1=$[CUR_SIZE1+1]
done

echo "echo \"Started $MAX_SIZE viceboxs\""              >>$VICE_SCRIPT
echo "$PM_PATH/powermand -f -c $PM_CONFIG --telemetry"  >>$PM_SCRIPT
echo "end nodes"                                        >>$PM_CONFIG

chmod 755 $VICE_SCRIPT
chmod 755 $PM_SCRIPT
