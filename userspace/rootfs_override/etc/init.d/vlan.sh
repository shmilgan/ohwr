#!/bin/sh

start() {
    echo -n "Setting up VLANs: "

    # set-up VLANs
    /wr/etc/vlan_config.sh

    if [ $? -eq 0 ]; then
	echo "OK"
    else
	echo "Failed"
    fi
}

stop() {
    echo -n "Clean VLANs' configuration: "

    /wr/bin/wrs_vlans --port 1-18 --pmode 3
    /wr/bin/wrs_vlans --clear

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
