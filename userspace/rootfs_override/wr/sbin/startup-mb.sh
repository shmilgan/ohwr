#!/bin/ash

echo "Starting up WR Switch (18-ports MiniBackplane version)..."
export WR_HOME="/wr"

$WR_HOME/bin/load-fpga $WR_HOME/lib/firmware/8ports_mb.bin
$WR_HOME/bin/lm32-loader $WR_HOME/lib/firmware/rt_cpu.bin
insmod $WR_HOME/lib/modules/wr_vic.ko
insmod $WR_HOME/lib/modules/wr-nic.ko
insmod $WR_HOME/lib/modules/wr_rtu.ko
$WR_HOME/sbin/start-daemons.sh