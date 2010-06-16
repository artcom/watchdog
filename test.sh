#!/bin/bash
exit 0;

if [[ "$1" == "" ]] ; then
    SCENE=testHeartbeat.js
else
    SCENE=""
fi
acxpshellOPT -I $PRO/src/y60/js $SCENE $*
