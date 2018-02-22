#!/bin/sh

if [ $# -ne 2 ]; then
    echo "Usage: $0 <output filename> <size>";
    exit 1;
else
    echo create file $1 with size $2.;
    dd if=/dev/urandom of=$1 bs=$2 count=1;
fi
