#!/bin/ash
export WR_HOME="/wr"

echo "Killing deamon"
killall wrsw_hal 
killall wrsw_rtud
killall ptpd

echo "Removing Kernel modules"
rmmod wr_rtu
rmmod wr-nic
rmmod wr_vic
rmmod at91_softpwm
