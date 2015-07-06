#!/bin/ash
export WR_HOME="/wr"

echo "Killing daemon"
killall wrsw_hal 
killall wrsw_rtud
killall ptpd

echo "Removing Kernel modules"
rmmod wr_pstats
rmmod wr_rtu
rmmod wr_nic
rmmod wr_vic
rmmod at91_softpwm
