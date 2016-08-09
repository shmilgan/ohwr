#!/bin/sh

# Adam Wujek @ CERN
#
# This script is to be run by udhcpc
#
# udhcpc should get hostname the from DHCP server.
#
# check whether we got hostname from DHCP server
if [ -n "$hostname" ]; then
    /bin/hostname "$hostname"
    echo "$hostname" > /etc/hostname
fi

# source default script
. /usr/share/udhcpc/default.script
