#!/bin/bash

# Write kernel and filesystem to NAND memory
echo "FLASHING: flashing kernel to /dev/mtd0 ..."
nandwrite -m -p -a /dev/mtd0 /flashing/zImage
echo "FLASHING: flashing file system to /dev/mtd1 ..."
nandwrite -m -p -a /dev/mtd1 /flashing/wrs-image.jffs2.img
echo "FLASHING: complete!"
# Reboot immediatly because we have completed the flashing procedure
reboot
