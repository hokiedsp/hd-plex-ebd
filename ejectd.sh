#! /bin/sh
### BEGIN INIT INFO
# Provides:          ejectd
# Required-Start:
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Eject Device Driver init script.
### END INIT INFO

##	Author:	Arsalan Masood
##	Email:	arslnmsd (at) gmail (dot) com

case "$1" in
	start)
		echo -n "Starting ejectd"
		/usr/sbin/ejectd
		echo "."
	;;
	stop)
		echo -n "Stopping ejectd"
		killall -e ejectd
		echo "."
	;;
	*)
		echo "Usage: /sbin/service ejectd {start|stop}"
		exit 1
esac

exit 0

