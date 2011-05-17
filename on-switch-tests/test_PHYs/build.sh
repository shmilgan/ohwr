#!/bin/sh

. ../../../settings

t=`pwd`
cd ../../libswitchhw
./build.sh clean
./build.sh
cd $t

make clean
make CROSS_COMPILE=$CC_CPU $1 $2 $3 $4
