#!/bin/sh

# Adam Wujek, CERN 2016
#
# Example of a custom script to be used at boot time.

start() {
    echo "$0: start"
}

stop() {
    echo "$0: stop"
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
    echo "Usage: $0 {start|stop|restart}"
    exit 1
    ;;
esac
