#!/bin/sh

start() {
    echo -n "Enable switching: "

    # bring up all interfaces
    for i in `ls /sys/class/net | grep wri`
    do
	    ifconfig $i up
    done

    echo "OK"
}

stop() {
    echo -n "Disable switching: "

    # bring down all interfaces
    for i in `ls /sys/class/net | grep wri`
    do
	    ifconfig $i down
    done

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
	;;
esac
