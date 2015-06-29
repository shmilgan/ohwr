#!/bin/ash
#
# Starts lighttpd daemon.
#

start_counter() {
	# increase boot counter
	COUNTER_FILE="/tmp/start_cnt_httpd"
	START_COUNTER=1
	if [ -f "$COUNTER_FILE" ];
	then
	    read -r START_COUNTER < $COUNTER_FILE
	    START_COUNTER=$((START_COUNTER+1))
	fi
	echo "$START_COUNTER" > $COUNTER_FILE
}

start() {
	echo -n "Starting lighttpd daemon: "
	# note start-stop-daemon does not create pidfile, only check if pid is
	# running
	start-stop-daemon -q -p /var/run/lighttpd.pid -S \
		--exec /usr/sbin/lighttpd -- -f /var/www/lighttpd.config
	ret=$?
	if [ $ret -eq 0 ]; then
		start_counter
		echo "OK"
	elif [ $ret -eq 1 ]; then
		echo "Failed (already running?)"
	else
		echo "Failed"
	fi
}

stop() {
	echo -n "Stopping lighttpd daemon: "
	start-stop-daemon -K -q -p /var/run/lighttpd.pid
	if [ $? -eq 0 ]; then
		echo "OK"
	else
		echo "Failed"
	fi
}

restart() {
	stop
	start
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
  *)
	echo $"Usage: $0 {start|stop|restart}"
	exit 1
esac

exit $?
