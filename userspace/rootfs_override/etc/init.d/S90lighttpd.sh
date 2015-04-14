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
	/usr/sbin/lighttpd -f /var/www/lighttpd.config
	start_counter
	echo "OK"
}
stop() {
	echo -n "Stopping lighttpd daemon: "
	killall lighttpd	
	echo "OK"
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
