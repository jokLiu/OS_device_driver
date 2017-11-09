#!/bin/bash

ret=0
d=/dev/opsysmem
executable=charDeviceDriver

function run(){
    echo -e "$1:"
    $1 #execute the command

    tmp=$? # get the return value from the execution
    if [ $tmp -ne 0 ]; then
        ret=$tmp
    fi
    echo "" #new line
    return $tmp
}


function test(){
    t="fsadfasdfasdfasdfasdfklasdhffffffffffffffffffffffffffffffffdjjjjjjjjjjfhajsdkfhaskdfjhaskdfhfdjfhasdkjfshklfasdkfhasdjfhsadjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjfhaksdjfhaskjdfhasdkjfhasdkjfhasdkjfhasdkjfhasdkjfhaskjdfhaskjdfhaskjdfhaskdjfhaksjdfhakjsdhfkfasjdh"

    #cleanup
    rmmod $executable > /dev/null 2>&1
    rm -f $d


    #load kernel module
    echo -en " load kernel driver:\t"
    insmod ./$executable.ko
    if [ $? -ne 0 ]; then
        echo "ERROR: insmod failed"
        return 1
    else
        echo "ok"
    fi

    #mknod
    echo -en " mknod:\t\t\t"
    major=`dmesg | tail -n 1| sed -n "s/\[[0-9. ]*\] 'mknod \/dev\/opsysmem c \([0-9]*\ 0'\./\1/p"`
    if [ $major -eq 0 ]
        echo -e "ERROR: major = 0, module ins't loaded"
        return 1
    fi
    mknod $d c $major 0
    if [ $? -ne 0 ]; then
        echo -e "ERROR: mkdnod command failes"
        rmmod $executable
        return 1
    else
        echo "ok"
    fi

    line=`ls $d 2> /dev/null`
    if [ $line != $d ]; then
        echo "ERROR"
        rmmod $executable
    else
        echo "ok"
    fi

    # multiwrite test
    echo -en "multi read and write test"
    for i in {1..50} ; do
        echo "$t" > $d 2> /dev/null
        if [ $? -ne 0 ]; then
            echo -e "Error writing to device, number: $i"
            return 1
        fi
    done

    #multiread test
}
