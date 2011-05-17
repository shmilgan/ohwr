#!/bin/sh

. ../../../settings

make CROSS_COMPILE=$CC_CPU $1 $2 $3 $4
