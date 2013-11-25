#!/bin/ash

echo "Starting up WR daemons..."

export WR_HOME="/wr"

$WR_HOME/bin/wrsw_hal -c $WR_HOME/etc/wrsw_hal.conf &> /dev/kmsg &
$WR_HOME/bin/wrsw_rtud >& /dev/kmsg &

# run ptp-noposix or ppsi, whatever is installed
if [ -x $WR_HOME/bin/ptpd ]; then
    $WR_HOME/bin/ptpd -A -c >& /dev/null &
    exit 0
fi
if [ -x $WR_HOME/bin/ppsi ]; then
    $WR_HOME/bin/pppsi >& /dev/kmsg &
    exit 0
fi
echo "No WR-PTP dameon found" >&2
exit 1


