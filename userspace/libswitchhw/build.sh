#!/bin/sh

. ../../settings

make CROSS_COMPILE=$CC_CPU $1
