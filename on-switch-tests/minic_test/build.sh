#!/bin/sh

. ../../../settings

cd ../../libswitchhw
./build.sh
cd ../tests/minic_test


cd ../../drivers/wr_minic
./build.sh 
cd ../../tests/minic_test

make CROSS_COMPILE=$CC_CPU $1 $2 $3 $4
