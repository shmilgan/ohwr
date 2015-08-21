#!/bin/sh

# simple wrapper for monit to announce reboot to console
echo "Monit triggered reboot due to $1" > /dev/console
echo "$1" > /update/monit_restart_reason
/sbin/reboot
