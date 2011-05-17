#!/bin/sh

. ../../../settings


make CROSS_COMPILE=$CROSS_COMPI_CPU $1 $2 $3 $4
