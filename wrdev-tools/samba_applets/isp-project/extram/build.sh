#!/bin/sh

make CROSS_COMPILE=/home/slayer/gnuarm-3.4.3/bin/arm-elf- CHIP=at91sam9263 BOARD=at91sam9263-ek MEMORIES=sram DYN_TRACES=1 INSTALLDIR=../../ clean all