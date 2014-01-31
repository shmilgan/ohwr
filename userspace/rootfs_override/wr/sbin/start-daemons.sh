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
if [ ! -x $WR_HOME/bin/ppsi ]; then
    echo "No WR-PTP daemon found" >&2
    exit 1
fi

# Fire ppsi. Unfortinately the hal takes a while to start, and
# until ppsi waits by itself (not in this submodule commit, yet),
# wait here...  This is the last script of the wr startup, and it can wait
for i in $(seq 1 10); do
    if [ -S /tmp/.minipc/wrsw_hal ]; then break; fi
    sleep 1
done
$WR_HOME/bin/ppsi >& /dev/kmsg &


