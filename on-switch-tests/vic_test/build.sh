#!/bin/sh

. ../../../settings

cd driver
./build.sh
cd ..

cd ../../libswitchhw
./build.sh
cd ../tests/vic_test

make CROSS_COMPILE=$CC_CPU $1 $2 $3 $4
