#!/bin/ash

echo "Starting up WR daemons..."

export WR_HOME="/wr"

$WR_HOME/bin/wrsw_hal -c $WR_HOME/etc/wrsw_hal.conf &> /dev/kmsg &
$WR_HOME/bin/wrsw_rtud >& /dev/kmsg &
$WR_HOME/bin/ptpd -A -c >& /dev/null &

