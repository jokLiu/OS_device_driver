#!/bin/bash

#globals
filetowrite=tmp/logs
filetowrite2=tmp/logss
ret=0
d=/dev/opsysmem
executable=charDeviceDriver

# --- helper function for parallel writing
function write_device(){
    for i in {10..99} ; do
        echo "$1$i" > $d
    done
}

# --- helper function for parallel reading
function read_device(){
    for i in {10..99} ; do
        head -n 1 < $d >> $filetowrite
    done
}

# --- helper function for testing
function test_file(){
    counter=1;
    for x in {a..z} ; do
        for i in {10..99} ; do
            line=$( sed -n "${counter}p" $filetowrite2 )
            if [ "$line" != "$x$i" ]
            then
                echo "failed with $line  and $x$ii\n"
                echo $counter
                return -1
            fi
            counter=$(( $counter + 1 ))
        done
    done
    return 0
}

# --- helper function ---
function run(){
    echo -e "$1:"
    $1 #execute
    #check errors
    tmp=$?
    if [ $tmp -ne 0 ]; then
        ret=$tmp
    fi
    echo "" #newline
    return $tmp
}


# --- TESTCASES ---
function basic_testcase(){
	t="testcase 1"
    gcc -D_POSIX_SOURCE -Wall -Werror -pedantic -std=c99 -D_GNU_SOURCE ioctl.c -o ioctl

	#cleanup
	rmmod $executable 2>/dev/null >/dev/null
	rm -f $d

	#load kernel driver
    echo -en " load kernel driver:\t"
	insmod ./$executable.ko
	if [ $? -ne 0 ]
	then
		echo -e "ERROR: insmod failed"
		return 1
    else
        echo "ok"
	fi

	#mknod
    echo -en " mknod:\t\t\t"
	major=`dmesg | tail -n 1 | sed -n "s/\[[0-9. ]*\] 'mknod \/dev\/opsysmem c \([0-9]*\) 0'\./\1/p"`
	if [ $major -eq 0 ]
	then
	    echo -e "ERROR: major = 0, probably the module isn't implemented"
	    rmmod $executeable
	    return 1
	fi
	mknod $d c $major 0
	if [ $? -ne 0 ]
	then
		echo -e "ERROR: mknod command failed"
		rmmod $executeable
		return 1
    else
        echo "ok"
	fi

    #check file
    echo -en " ls $d:\t"
	line=`ls $d 2>/dev/null`
	if [ "$line" != "$d" ]
	then
		echo -e "ERROR: file $d does not exist after loading the kernel module"
		rmmod $executable
		return 1
    else
        echo "ok"
	fi

	#write test
    echo -en " write test:\t\t"
	echo "$t" > $d 2>/dev/null
	if [ $? -ne 0 ]
	then
		echo -e "ERROR: writing $d failed"
		rmmod $executable
		return 1
    else
        echo "ok"
	fi

	#read test
    echo -en " read test:\t\t"
	r=`head -n 1 < $d`
	if [ $? -ne 0 ]
	then
		echo -e "ERROR: reading $d failed"
		rmmod $executable
		return 1
	fi

	#check if same was read
	if [ "$r" != "$t" ]
	then
		echo -e "ERROR: $d: could not read what was written before"
		rmmod $executable
		return 1
    else
        echo "ok"
	fi

    #multi write test
    echo -en " multi write test\t\t"
    for x in {a..z} ; do
        write_device $x &
    done

    echo "sleeping"
    sleep 40

    #now read to the file in parallel
    for x in {a..z} ; do
        read_device &
    done

    echo "sleeping"
    sleep 150

    #sort the file
    cat $filetowrite | sort > $filetowrite2
    test_file
    if [ $? -ne 0 ]
    then
        echo -e "ERROR: parallel test failed"
        return 1
    else
        echo "ok"
    fi


    #unavailable message test
    echo -en "unavailable message test\t\t"
    for x in {1..100} ; do
        head -n 1 < $d > /dev/null 2&>1
        if [ $? -eq 0 ] ; then
            echo -e "ERROR: returned different answer, no message should be available"
            return 1
        fi
    done
    echo "ok"

    #ioctl test
    maxsize=`dmesg | tail -n 1 | sed -n "s/\[[0-9. ]*\] 'size \([0-9]*\) '\./ \1/p"`
    echo $maxsize
    if [ $maxsize -ne 2097152 ] ; then
        echo "wrong max msg size"
        return 1
    fi

    ./ioctl $d 100
    maxsize=`dmesg | tail -n 1 | sed -n "s/\[[0-9. ]*\] 'size \([0-9]*\) '\./ \1/p"`
    if [ $maxsize -ne 100 ] ; then
        echo "wrong max msg size"
        return 1
    fi
    echo "ok"

    for i in {1..100} ; do
        echo $i > $d
    done

    echo 1 > $d
    if [ $? -ne -11 ] ; then
        echo "failed, should not e ellowed to write"
        return 1
    fi
    echo "ok"

	#unload module
    echo -en " unload module:\t\t"
	rmmod $executable
	if [ $? -ne 0 ]
	then
		echo -e "ERROR: unloading kernel module failed"
		return 1
    else
        echo "ok"
	fi

	return 0
}

# --- execution ---
#reset
rmmod $executable 2>/dev/null
rm tmp/*
run basic_testcase
#cleanup
rm -f $d

exit $ret

