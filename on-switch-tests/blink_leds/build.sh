#!/bin/sh

. ../../../settings
#cd ../../libswitchhw
#./build.sh
#cd ../tests/blink_leds


make CROSS_COMPILE=$CC_CPU $1 $2 $3 $4
