#!/bin/bash

# Setup for test
dmesg -c > /dev/null
insmod rtap.ko

# Validate
if [ ! -d /proc/rtap ]; then
    echo "Error inserting module: missing rtap directory"
    rmmod rtap
    exit 1
fi
if [ ! -e /proc/rtap/listeners ]; then
    echo "Error inserting module: missing listeners file"
    rmmod rtap
    exit 1
fi
if [ ! -e /proc/rtap/devices ]; then
    echo "Error inserting module: missing devices file"
    rmmod rtap
    exit 1
fi
if [ ! -e /proc/rtap/filters ]; then
    echo "Error inserting module: missing filters file"
    rmmod rtap
    exit 1
fi

# Configure test
echo -n "127.0.0.1" > /proc/rtap/listeners
echo -n "mon0" > /proc/rtap/devices

# Forwarding test
echo "1 3 2 2 F0:25:B7:00:B5:33" > /proc/rtap/filters
sleep 5
echo -1 > /proc/rtap/filters

# Validate
dmesg -c > /tmp/dmesg.log
if grep "RTAP: Forwarding" /tmp/dmesg.log; then
    echo "Pass 1"
else
    echo "Fail 1"
fi

# Drop test
echo "2 3 2 1 F0:25:B7:00:B5:33" > /proc/rtap/filters
sleep 5
echo -2 > /proc/rtap/filters

# Validate
dmesg -c > /tmp/dmesg.log
if grep "RTAP: Dropping" /tmp/dmesg.log; then
    echo "Pass 2"
else
    echo "Fail 2"
fi

# Cleanup
rmmod rtap




