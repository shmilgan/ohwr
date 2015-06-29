#!/bin/sh

# simple wrapper for monit to announce reboot to console
echo "Monit triggered reboot due to $1" > /dev/console
/sbin/reboot
