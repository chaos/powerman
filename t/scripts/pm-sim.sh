#!/bin/bash

#
# pm-sim - simulate just enough powerman to test powerman-stonith script
#
declare -r prog=pm-sim
aopt=0
Aopt=0
onopt=0
offopt=0
queryopt=0
targ=""
t1=off
t2=off
t3=off
t=off
ropt=0
sopt=0
topt=0

simstate=$prog.data
[ -r $simstate ] && source $simstate

PATH=/usr/bin:/bin:$PATH

die()
{
    echo "${prog}: $1" >&2
    exit 1
}

usage()
{
    echo "Usage: ${prog} OPTIONS [-x] [-q|-0|-1] target" 2>&1
    echo "where OPTIONS are:" 2>&1
    echo "  -a               alias t t[1-2]" 2>&1
    echo "  -A               alias t t[1-3]" 2>&1
    echo "  -m off|on        set t1 initial state" 2>&1
    echo "  -n off|on        set t2 initial state" 2>&1
    echo "  -o off|on        set t3 initial state" 2>&1
    echo "  -r               make t1 stuck" 2>&1
    echo "  -s               make t2 stuck" 2>&1
    echo "  -t               make t3 stuck" 2>&1
    exit 1
}

[ $# == 0 ] && usage
while getopts "1:0:q:aAxm:n:o:rst" opt; do
    case ${opt} in
        1) onopt=1; targ=${OPTARG} ;;
        0) offopt=1; targ=${OPTARG} ;;
        q) queryopt=1; targ=${OPTARG} ;;
        x) ;;
        a) aopt=1 ;;
        A) Aopt=1 ;;
        m) t1=${OPTARG} ;;
        n) t2=${OPTARG} ;;
        o) t3=${OPTARG} ;;
        r) ropt=1 ;;
        s) sopt=1 ;;
        t) topt=1 ;;
        *) usage ;;
    esac
done
shift $((${OPTIND} - 1))

if [ "$onopt" == 1 ]; then
    [ $ropt == 1 ] || t1=on
    [ $sopt == 1 ] || t2=on
    [ $topt == 1 ] || t3=on
    echo Command completed successfully
elif [ "$offopt" == 1 ]; then
    [ $ropt == 1 ] || t1=off
    [ $sopt == 1 ] || t2=off
    [ $topt == 1 ] || t3=off
    echo Command completed successfully
elif [ "$queryopt" == 1 ]; then
    if [ "$aopt" == 1 ]; then
        echo "t1: $t1"
        echo "t2: $t2"
    elif [ "$Aopt" == 1 ]; then
        echo "t1: $t1"
        echo "t2: $t2"
        echo "t3: $t3"
    else
        if [ $t1 == on ] && [ $t2 == on ] && [ $t3 == on ]; then
            echo "t: on"
        elif [ $t1 == off ] && [ $t2 == off ] && [ $t3 == off ]; then
            echo "t: off"
        else
            echo "t: unknown"
        fi
    fi
fi

(echo t1=$t1; echo t2=$t2; echo t3=$t3) >$simstate

exit 0
