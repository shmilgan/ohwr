#!/bin/sh

set -e
## vlan configuration
## fill this file with vlans configuration

## Please note:
## wr1 is marked as port 2 on the front panel of the switch
## in other words:
## --ep1 == wr1
## but:
## wr1 == port2

## example configuration:
# configure ports 1-4,6,8-18 as evid 1
# /wr/bin/wrs_vlans --ep 0-3,5,7-17  --emode 0 --evid 1
# configure ports 5,7 as evid 2
# /wr/bin/wrs_vlans --ep 4,6  --emode 0 --evid 2

## set VID=1 on all ports except 4 and 6
## - binary: 11 1111 1111 1010 1111
## - hex   : 0x3FFAF
# /wr/bin/wrs_vlans --rvid 1 --rfid 1 --rmask 0x3FFAF

## set VID=2 at ports 5 and 7 - this is 
## - binary: 00 0000 0000 0101 0000
## - hex   : 0x00050
# /wr/bin/wrs_vlans --rvid 2 --rfid 2 --rmask 0x00050

## when VLANs are defined, comment out the line below
echo -n "no configuration "

exit 0
