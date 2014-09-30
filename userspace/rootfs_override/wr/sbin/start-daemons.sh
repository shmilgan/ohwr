#!/bin/ash
echo "Starting up WR daemons..."

# The following variables are modified at build time, with Kconfig values
LOG_HAL=CONFIG_WRS_LOG_HAL
LOG_RTU=CONFIG_WRS_LOG_RTU
LOG_PTP=CONFIG_WRS_LOG_PTP

# The loop below is a little intricate because of the eval, but I need it this
# to have pipe and file as both supported.
# Also, to have both stdout and stderr, we need "2>&1" before the pipe, but
# after the file redirection (why????)

for n in HAL RTU PTP; do
    # if empty turn it to /dev/null
    if eval test -z "\$LOG_$n"; then LOG_$n="/dev/null"; fi
    # if a pathname, use it
    eval value="\$LOG_$n"
    if echo "$value" | grep / > /dev/null; then
	eval LOGPIPE_$n=\" \> $value 2\>\&1 \";
	continue;
    fi
    # not a pathname: use verbatim
    eval LOGPIPE_$n=\" 2\>\&1 \| logger -t wr-switch -p $value\"
done

export WR_HOME="/wr"

eval $WR_HOME/bin/wrsw_hal -c $WR_HOME/etc/wrsw_hal.conf $LOGPIPE_HAL \&
eval $WR_HOME/bin/wrsw_rtud                              $LOGPIPE_RTU \&

# run ptp-noposix or ppsi, whatever is installed
if [ -x $WR_HOME/bin/ptpd ]; then
    eval $WR_HOME/bin/ptpd -A -c  $LOGPIPE_PTP \&
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
eval $WR_HOME/bin/ppsi  $LOGPIPE_PTP \&

# ensure we receive UDP PTP frames, since ppsi supports UDP too.
(sleep 4; $WR_HOME/bin/rtu_stat add 01:00:5e:00:01:81 18 0) &

