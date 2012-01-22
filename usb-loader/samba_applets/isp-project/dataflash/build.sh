#!/bin/sh

make CROSS_COMPILE=/opt/gcc-codesourcery/bin/arm-none-eabi- CHIP=at91sam9g45 BOARD=at91sam9g45-ek MEMORIES=sram TRACE_LEVEL=5 DYN_TRACES=1 INSTALLDIR=../../ $1