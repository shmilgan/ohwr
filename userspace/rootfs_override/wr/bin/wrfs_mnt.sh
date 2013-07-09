#!/bin/ash

if [ ! $# -eq 1 ]; then
	echo "ro/rw paremeter needed"
	exit 0
fi

if [ $1 = "rw" ]; then
	echo "Remounting rootfs read/write"
	mount -o remount,rw,sync /
fi

if [ $1 = "ro" ]; then
	echo "Remounting rootfs read-only"
	sync
	mount -o remount,ro,async /
fi
