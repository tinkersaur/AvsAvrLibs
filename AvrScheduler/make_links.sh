#!/bin/bash

# Make hard links in each test directory AvrScheduler/*test* pointing to the files at AvrScheduler/*.

L=`ls -x`
for F in $L ; do
    if test "$F" != docs -a  "$F" != AvrScheduler \
       -a  "$F" != AvrScheduler2 -a -d "$F"; then
        pushd "$F" > /dev/null
        echo pwd = `pwd`
        echo linking: ln -f ../AvrScheduler/* .
        ln -f ../AvrScheduler/* .
        popd > /dev/null
    fi 
done
