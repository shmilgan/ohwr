#!/bin/ash

echo "Starting up WR Switch (18-ports MiniBackplane version)..."
export WR_HOME="/wr"

# Get parameter from kernel commandline
for arg in $(cat /proc/cmdline); do
	echo $arg | grep -q "wr_nic.macaddr" ;
	if [ $? == 0 ]; then
		val=$(echo $arg | cut -d= -f2);
	fi;
done


$WR_HOME/bin/load-virtex $WR_HOME/lib/firmware/8ports_mb-tru-bigFPGA.bin
#$WR_HOME/bin/load-virtex $WR_HOME/lib/firmware/8ports_mb-tru.bin
#$WR_HOME/bin/load-virtex $WR_HOME/lib/firmware/18ports_mb.bin
$WR_HOME/bin/load-lm32 $WR_HOME/lib/firmware/rt_cpu.bin
insmod $WR_HOME/lib/modules/at91_softpwm.ko
insmod $WR_HOME/lib/modules/wr_vic.ko
insmod $WR_HOME/lib/modules/wr-nic.ko macaddr=$val
insmod $WR_HOME/lib/modules/wr_rtu.ko
$WR_HOME/sbin/start-daemons.sh
