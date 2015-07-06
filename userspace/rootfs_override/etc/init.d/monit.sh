#!/bin/sh

# script to be run directly from init

dotconfig=/wr/etc/dot-config
monit_pid=/var/run/monit.pid

start() {
    # to start monit, first kill monit running/stopped previously, init will
    # take care of starting it again
    if [ -f $monit_pid ]; then
	kill -9 `cat $monit_pid`
    else
	echo "Unable to find monit's pid"
	exit 1
    fi
}

stop() {
    echo "Stopping monit"
    # Stop monit by sending STOP singal to it. Killing monit will make init
    # to start another instance of monit.
    if [ -f $monit_pid ]; then
	kill -STOP `cat $monit_pid`
    else
	echo "Unable to find monit's pid"
	exit 1
    fi

}

restart() {
    echo "Restarting monit"
    # invoke start
    start
}

loop_forever() {
    # Save pid of current script as monit's (used by start function).
    echo $$ > $monit_pid
    # sleep forever
    while true; do sleep 10000; done
}

init() {
    # First, read dot-config to know if we shall start monit
    if [ -f $dotconfig ]; then
	. $dotconfig
    else
	echo "dot-config not found! Don't start monit"
	loop_forever
    fi

    if [ -z "$CONFIG_MONIT_DISABLE" ]; then

	if [ -z $CONFIG_WRS_LOG_MONIT ]; then
	    LOG="";
	elif echo "$CONFIG_WRS_LOG_MONIT" | grep / > /dev/null; then
	    # if a pathname, use it
	    LOG="-l $CONFIG_WRS_LOG_MONIT";
	else
	    # not a pathname, monit cannot take facility nor level from command
	    # line, only possible from configuration file
	    LOG="-l syslog"
	fi

	echo "Start monit"
	# start monit
	/usr/bin/monit $LOG
    else
	echo "Monit disabled in dot-config"
	loop_forever
    fi

}

case "$1" in
    start)
	start
	;;
    stop)
	stop
	;;
    restart|reload)
	restart
	;;
    init)
	init
	;;
    *)
	echo $"Usage: $0 {start|stop|restart|init}"
	exit 1
	;;
esac
