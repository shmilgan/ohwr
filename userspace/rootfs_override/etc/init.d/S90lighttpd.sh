#!/bin/ash
#
# Starts lighttpd daemon.
#
start() {
 	echo -n "Starting lighttpd daemon: "
	/usr/sbin/lighttpd -f /var/www/lighttpd.config
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
