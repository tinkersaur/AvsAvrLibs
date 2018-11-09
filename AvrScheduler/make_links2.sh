#!/bin/bash

# Make hard links in each test directory AvrScheduler/*test* pointing to the files at AvrScheduler/*.

L=`ls -x`
for F in $L ; do
    if test -d "$F" \
       -a  "$F" != AvrScheduler \
       -a  "$F" != AvrScheduler2 \
       -a  "$F" != 080_test_timer_setup \
       -a  "$F" != docs; then
        pushd "$F" > /dev/null
        echo pwd = `pwd`
        echo linking: ln -f ../AvrScheduler2/* .
        ln -f ../AvrScheduler2/* .
        popd > /dev/null
    fi 
done
