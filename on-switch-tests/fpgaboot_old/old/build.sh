#!/bin/sh

. ../build.settings


make CROSS_COMPILE=$CC_PATH/$CC_PREFIX $1
